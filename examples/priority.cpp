//          Copyright Nat Goodspeed 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
#include <iostream>

#include <boost/fiber/all.hpp>
#include <boost/noncopyable.hpp>

class Verbose: public boost::noncopyable {
public:
    Verbose( std::string const& d, std::string const& s="stop") :
        desc( d),
        stop( s) {
        std::cout << desc << " start" << std::endl;
    }

    ~Verbose() {
        std::cout << desc << " " << stop << std::endl;
    }

private:
    std::string     desc;
    std::string     stop;
};

class Verbose : public boost::noncopyable {
public:
    Verbose( std::string const& d, std::string const& s = "stop") :
        desc( d),
        stop( s) {
        std::cout << desc << " start" << std::endl;
    }

    ~Verbose() {
        std::cout << desc << " " << stop << std::endl;
    }

private:
    std::string desc, stop;
};

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

        // Find 'f' in the queue. Note that it might not be in our queue at
        // all, if caller is changing the priority of (say) the running fiber.
        bool found = false;
        for ( boost::fibers::fiber_context ** fp = & head_; * fp; fp = & ( * fp)->nxt) {
            if ( * fp == f) {
                // found the passed fiber in our list -- unlink it
                found = true;
                * fp = ( * fp)->nxt;
                f->nxt = nullptr;
                break;
            }
        }

        // It's possible to get a property_change() call for a fiber that is
        // not on our ready queue. If it's not there, no need to move it:
        // we'll handle it next time it hits awakened().
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

        // Here we know that f was in our ready queue, but we've unlinked it.
        // We happen to have a method that will (re-)add a fiber_context* to
        // the ready queue.
        awakened(f, props);
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

//[launch
template< typename Fn >
boost::fibers::fiber launch( Fn && func, std::string const& name, int priority) {
    boost::fibers::fiber fiber( func);
    priority_props & props( fiber.properties< priority_props >() );
    props.name = name;
    props.set_priority( priority);
    return fiber;
}
//]

void yield_fn() {
    std::string name( boost::this_fiber::properties< priority_props >().name);
    Verbose v( std::string("fiber ") + name);
    for ( int i = 0; i < 3; ++i) {
        std::cout << "fiber " << name << " yielding" << std::endl;
        boost::this_fiber::yield();
    }
}

void barrier_fn( boost::fibers::barrier & barrier) {
    std::string name( boost::this_fiber::properties< priority_props >().name);
    Verbose v( std::string("fiber ") + name);
    std::cout << "fiber " << name << " waiting on barrier" << std::endl;
    barrier.wait();
    std::cout << "fiber " << name << " yielding" << std::endl;
    boost::this_fiber::yield();
}

//[change_fn
void change_fn( boost::fibers::fiber & other,
                int other_priority,
                boost::fibers::barrier& barrier) {
    std::string name( boost::this_fiber::properties< priority_props >().name);
    Verbose v( std::string("fiber ") + name);

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
}
//]

//[main
int main( int argc, char *argv[]) {
    // make sure we use our priority_scheduler rather than default round_robin
    boost::fibers::use_scheduling_algorithm< priority_scheduler >();
/*=    ...*/
/*=}*/
//]
    Verbose v("main()");

    // for clarity
    std::cout << "main() setting name" << std::endl;
//[main_name
    boost::this_fiber::properties< priority_props >().name = "main";
//]
    std::cout << "main() running tests" << std::endl;

    {
        Verbose v("high-priority first", "stop\n");
        // verify that high-priority fiber always gets scheduled first
        boost::fibers::fiber low( launch( yield_fn, "low",    1) );
        boost::fibers::fiber med( launch( yield_fn, "medium", 2) );
        boost::fibers::fiber hi( launch( yield_fn,  "high",   3) );
        std::cout << "main: high.join()" << std::endl;
        hi.join();
        std::cout << "main: medium.join()" << std::endl;
        med.join();
        std::cout << "main: low.join()" << std::endl;
        low.join();
    }

    {
        Verbose v("same priority round-robin", "stop\n");
        // fibers of same priority are scheduled in round-robin order
        boost::fibers::fiber a( launch( yield_fn, "a", 0) );
        boost::fibers::fiber b( launch( yield_fn, "b", 0) );
        boost::fibers::fiber c( launch( yield_fn, "c", 0) );
        std::cout << "main: a.join()" << std::endl;
        a.join();
        std::cout << "main: b.join()" << std::endl;
        b.join();
        std::cout << "main: c.join()" << std::endl;
        c.join();
    }

    {
        Verbose v("barrier wakes up all", "stop\n");
        // using a barrier wakes up all waiting fibers at the same time
        boost::fibers::barrier barrier( 3);
        boost::fibers::fiber low( launch( [&barrier](){ barrier_fn( barrier); }, "low",    1) );
        boost::fibers::fiber med( launch( [&barrier](){ barrier_fn( barrier); }, "medium", 2) );
        boost::fibers::fiber hi( launch( [&barrier](){ barrier_fn( barrier); },  "high",   3) );
        std::cout << "main: low.join()" << std::endl;
        low.join();
        std::cout << "main: medium.join()" << std::endl;
        med.join();
        std::cout << "main: high.join()" << std::endl;
        hi.join();
    }

    {
        Verbose v("change priority", "stop\n");
        // change priority of a fiber in priority_scheduler's ready queue
        boost::fibers::barrier barrier( 3);
        boost::fibers::fiber c( launch( [&barrier](){ barrier_fn( barrier); }, "c", 1) );
        boost::fibers::fiber a( launch( [&c,&barrier]() { change_fn( c, 3, barrier); }, "a", 3) );
        boost::fibers::fiber b( launch( [&barrier](){ barrier_fn( barrier); }, "b", 2) );
        std::cout << "main: a.join()" << std::endl;
        std::cout << "main: a.join()" << std::endl;
        a.join();
        std::cout << "main: b.join()" << std::endl;
        b.join();
        std::cout << "main: c.join()" << std::endl;
        c.join();
    }

    return EXIT_SUCCESS;
}
