//          Copyright Nat Goodspeed 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/all.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <iostream>

//[priority_props
class priority_props : public boost::fibers::fiber_properties {
public:
    priority_props( boost::fibers::fiber_context * p):
        fiber_properties( p), /*< Your subclass constructor must accept a 
                                 [^[class_link fiber_context]*] and pass it to
                                 the `fiber_properties` constructor. >*/
        priority_( 0) {
    }

    int get_priority() const {
        return priority_; /*< Provide read access methods at your own discretion. >*/
    }

    // Call this method to alter priority, because we must notify
    // priority_scheduler of any change.
    void set_priority( int p) {
        /*< It's important to call notify() on any
            change in a property that can affect the
            scheduler's behavior. Therefore, such
            modifications should only be performed
            through an access method. >*/
        // Of course, it's only worth reshuffling the queue and all if we're
        // actually changing the priority.
        if ( p != priority_) {
            priority_ = p;
            notify();
        }
    }

    // The fiber name of course is solely for purposes of this example
    // program; it has nothing to do with implementing scheduler priority.
    // This is a public data member -- not requiring set/get access methods --
    // because we need not inform the scheduler of any change.
    std::string name; /*< A property that does not affect the scheduler does
                          not need access methods. >*/
private:
    int priority_;
};
//]

//[priority_scheduler
class priority_scheduler : public boost::fibers::sched_algorithm_with_properties< priority_props > {
private:
    // Much as we would like, we don't use std::priority_queue because it
    // doesn't appear to provide any way to alter the priority (and hence
    // queue position) of a particular item.
    boost::fibers::fiber_context    *   head_;

public:
    priority_scheduler() :
        head_( nullptr) {
    }

    // For a subclass of sched_algorithm_with_properties<>, it's important to
    // override the correct awakened() overload.
    /*<< You must override the [member_link sched_algorithm_with_properties..awakened]
         method. This is how your scheduler receives notification of a
         fiber that has become ready to run. >>*/
    virtual void awakened( boost::fibers::fiber_context * f, priority_props & props) {
        int f_priority = props.get_priority(); /*< `props` is the instance of
                                                   priority_props associated
                                                   with the passed fiber `f`. >*/
        // With this scheduler, fibers with higher priority values are
        // preferred over fibers with lower priority values. But fibers with
        // equal priority values are processed in round-robin fashion. So when
        // we're handed a new fiber_base, put it at the end of the fibers with
        // that same priority. In other words: search for the first fiber in
        // the queue with LOWER priority, and insert before that one.
        boost::fibers::fiber_context ** fp = & head_;
        for ( ; * fp; fp = & ( * fp)->nxt) {
            if ( properties( * fp).get_priority() < f_priority) {
                /*< Use the
                [member_link sched_algorithm_with_properties..properties]
                method to access properties for any ['other] fiber. >*/
                break;
            }
        }
        // It doesn't matter whether we hit the end of the list or found
        // another fiber with lower priority. Either way, insert f here.
        f->nxt = * fp; /*< Note use of the [data_member_link fiber_context..nxt] member. >*/
        * fp = f;
//<-

        std::cout << "awakened(" << props.name << "): ";
        describe_ready_queue();
//->
    }

    /*<< You must override the [member_link sched_algorithm_with_properties..pick_next]
         method. This is how your scheduler actually advises the fiber manager
         of the next fiber to run. >>*/
    virtual boost::fibers::fiber_context * pick_next() {
        // if ready queue is empty, just tell caller
        if ( ! head_) {
            return nullptr;
        }
        // Here we have at least one ready fiber. Unlink and return that.
        boost::fibers::fiber_context * f = head_;
        head_ = f->nxt;
        f->nxt = nullptr;

//<-
        std::cout << "pick_next() resuming " << properties( f).name << ": ";
        describe_ready_queue();
//->
        return f;
    }

    /*<< You must override [member_link sched_algorithm_with_properties..ready_fibers]
      to inform the fiber manager of the size of your ready queue. >>*/
    virtual std::size_t ready_fibers() const noexcept {
        std::size_t count = 0;
        for ( boost::fibers::fiber_context * f = head_; f; f = f->nxt) {
            ++count;
        }
        return count;
    }

    /*<< Overriding [member_link sched_algorithm_with_properties..property_change]
         is optional. This override handles the case in which the running
         fiber changes the priority of another ready fiber: a fiber already in
         our queue. In that case, move the updated fiber within the queue. >>*/
    virtual void property_change( boost::fibers::fiber_context * f, priority_props & props) {
        // Although our priority_props class defines multiple properties, only
        // one of them (priority) actually calls notify() when changed. The
        // point of a property_change() override is to reshuffle the ready
        // queue according to the updated priority value.
//<-
        std::cout << "property_change(" << props.name << '(' << props.get_priority()
                  << ")): ";
//->

        // Despite the added complexity of the loop body, make a single pass
        // over the queue to find both the existing item and the new desired
        // insertion point.
        bool found = false;
        boost::fibers::fiber_context ** insert = nullptr, ** fp = & head_;
        for ( ; * fp; fp = & ( * fp)->nxt) {
            if ( * fp == f) {
                // found the passed fiber in our list -- unlink it
                found = true;
                * fp = ( * fp)->nxt;
                f->nxt = nullptr;
                // If that was the last item in the list, stop.
                if ( ! * fp) {
                    break;
                }
                // If we've already found the new insertion point, no need to
                // continue looping.
                if ( insert) {
                    break;
                }
            }
            // As in awakened(), we're looking for the first fiber in the
            // queue with priority lower than the passed fiber.
            if ( properties( * fp).get_priority() < props.get_priority() ) {
                insert = fp;
                // If we've already found and unlinked the passed fiber, no
                // need to continue looping.
                if ( found) {
                    break;
                }
            }
        }
        // property_change() should only be called if f->is_ready(). However,
        // a waiting fiber can change state to is_ready() while still on the
        // fiber_manager's waiting queue. Every such fiber will be swept onto
        // our ready queue before the next pick_next() call, but still it's
        // possible to get a property_change() call for a fiber that
        // is_ready() but is not yet on our ready queue. If it's not there, no
        // action required: we'll handle it next time it hits awakened().
        if ( ! found) {
            /*< Your `property_change()` override must be able to
            handle the case in which the passed `f` is not in
            your ready queue. It might be running, or it might be
            blocked. >*/
//<-
            // hopefully user will distinguish this case by noticing that
            // the fiber with which we were called does not appear in the
            // ready queue at all
            describe_ready_queue();
//->
            return;
        }
        // There might not be any ready fibers with lower priority. In that
        // case, append to the end of the queue.
        /*=if (! insert)*/
//<-
        std::string where;
        if ( insert) {
            where = std::string("before ") + properties( * insert).name;
        } else {
//->
            insert = fp;
//<-
            where = "to end";
//->
        }
        // Insert f at the new insertion point in the queue.
        f->nxt = * insert;
        * insert = f;
//<-

        std::cout << "moving " << where << ": ";
        describe_ready_queue();
//->
    }
//<-

    void describe_ready_queue() {
        if ( ! head_) {
            std::cout << "[empty]";
        } else {
            const char * delim = "";
            for ( boost::fibers::fiber_context * f = head_; f; f = f->nxt) {
                priority_props & props( properties( f) );
                std::cout << delim << props.name << '(' << props.get_priority() << ')';
                delim = ", ";
            }
        }
        std::cout << std::endl;
    }
//->
};
//]

//[init
void init( std::string const& name, int priority) {
    priority_props & props(
            boost::this_fiber::properties< priority_props >() );
    props.name = name;
    props.set_priority( priority);
}
//]

void yield_fn( std::string const& name, int priority) {
    init( name, priority);

    for ( int i = 0; i < 3; ++i) {
        std::cout << "fiber " << name << " running" << std::endl;
        boost::this_fiber::yield();
    }
}

void barrier_fn( std::string const& name, int priority, boost::fibers::barrier & barrier) {
    init( name, priority);

    std::cout << "fiber " << name << " waiting on barrier" << std::endl;
    barrier.wait();
    std::cout << "fiber " << name << " yielding" << std::endl;
    boost::this_fiber::yield();
    std::cout << "fiber " << name << " done" << std::endl;
}

//[change_fn
void change_fn( std::string const& name,
                int priority,
                boost::fibers::fiber & other,
                int other_priority,
                boost::fibers::barrier& barrier) {
    init( name, priority);

//<-
    std::cout << "fiber " << name << " waiting on barrier" << std::endl;
//->
    barrier.wait();
    // We assume a couple things about 'other':
    // - that it was also waiting on the same barrier
    // - that it has lower priority than this fiber.
    // If both are true, 'other' is now ready to run but is sitting in
    // priority_scheduler's ready queue. Change its priority.
    priority_props & other_props(
            other.properties< priority_props >() );
//<-
    std::cout << "fiber " << name << " changing priority of " << other_props.name
              << " to " << other_priority << std::endl;
//->
    other_props.set_priority( other_priority);
//<-
    std::cout << "fiber " << name << " done" << std::endl;
//->
}
//]

//[main
int main( int argc, char *argv[]) {
    // make sure we use our priority_scheduler rather than default round_robin
    boost::fibers::use_scheduling_algorithm< priority_scheduler >();
/*=    ...*/
/*=}*/
//]

    {
        // verify that high-priority fiber always gets scheduled first
        boost::fibers::fiber low( boost::bind( yield_fn, "low",    1) );
        boost::fibers::fiber med( boost::bind( yield_fn, "medium", 2) );
        boost::fibers::fiber hi( boost::bind( yield_fn,  "high",   3) );
        hi.join();
        med.join();
        low.join();
        std::cout << std::endl;
    }

    {
        // fibers of same priority are scheduled in round-robin order
        boost::fibers::fiber a( boost::bind( yield_fn, "a", 0) );
        boost::fibers::fiber b( boost::bind( yield_fn, "b", 0) );
        boost::fibers::fiber c( boost::bind( yield_fn, "c", 0) );
        a.join();
        b.join();
        c.join();
        std::cout << std::endl;
    }

    {
        // using a barrier wakes up all waiting fibers at the same time
        boost::fibers::barrier barrier( 3);
        boost::fibers::fiber low( boost::bind( barrier_fn, "low",    1, boost::ref( barrier) ) );
        boost::fibers::fiber med( boost::bind( barrier_fn, "medium", 2, boost::ref( barrier) ) );
        boost::fibers::fiber hi( boost::bind( barrier_fn,  "high",   3, boost::ref( barrier) ) );
        low.join();
        med.join();
        hi.join();
        std::cout << std::endl;
    }

    {
        // change priority of a fiber in priority_scheduler's ready queue
        boost::fibers::barrier barrier( 3);
        boost::fibers::fiber c( boost::bind( barrier_fn, "c", 1, boost::ref( barrier) ) );
        boost::fibers::fiber a( boost::bind( change_fn,  "a", 3, boost::ref( c), 3, boost::ref( barrier) ) );
        boost::fibers::fiber b( boost::bind( barrier_fn, "b", 2, boost::ref( barrier) ) );
        a.join();
        b.join();
        c.join();
        std::cout << std::endl;
    }

    std::cout << "done." << std::endl;

    return EXIT_SUCCESS;
}
