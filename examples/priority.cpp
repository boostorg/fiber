//          Copyright Nat Goodspeed 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/all.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <iostream>

class priority_props: public boost::fibers::fiber_properties
{
public:
    priority_props(boost::fibers::fiber_properties::back_ptr p):
        fiber_properties(p),
        priority_(0)
    {}

    int get_priority() const { return priority_; }

    // Call this method to alter priority, because we must notify
    // priority_scheduler of any change.
    void set_priority(int p)
    {
        // Of course, it's only worth reshuffling the queue and all if we're
        // actually changing the priority.
        if (p != priority_)
        {
            priority_ = p;
            notify();
        }
    }

    // The fiber name of course is solely for purposes of this example
    // program; it has nothing to do with implementing scheduler priority.
    // This is a public data member -- not requiring set/get access methods --
    // because we need not inform the scheduler of any change.
    std::string name;

private:
    int priority_;
};

class priority_scheduler:
    public boost::fibers::sched_algorithm_with_properties<priority_props>
{
private:
    // Much as we would like, we don't use std::priority_queue because it
    // doesn't appear to provide any way to alter the priority (and hence
    // queue position) of a particular item.
    boost::fibers::fiber_context* head_;

public:
    priority_scheduler():
        head_(nullptr)
    {}

    // For a subclass of sched_algorithm_with_properties<>, it's important to
    // override awakened_props(), NOT awakened().
    virtual void awakened_props(boost::fibers::fiber_context* f)
    {
        int f_priority = properties(f).get_priority();
        // With this scheduler, fibers with higher priority values are
        // preferred over fibers with lower priority values. But fibers with
        // equal priority values are processed in round-robin fashion. So when
        // we're handed a new fiber_base, put it at the end of the fibers with
        // that same priority. In other words: search for the first fiber in
        // the queue with LOWER priority, and insert before that one.
        boost::fibers::fiber_context** fp = &head_;
        for ( ; *fp; fp = &(*fp)->nxt)
            if (properties(*fp).get_priority() < f_priority)
                break;
        // It doesn't matter whether we hit the end of the list or found
        // another fiber with lower priority. Either way, insert f here.
        f->nxt = *fp;
        *fp = f;

        std::cout << "awakened(" << properties(f).name << "): ";
        describe_ready_queue();
    }

    virtual boost::fibers::fiber_context* pick_next()
    {
        // if ready queue is empty, just tell caller
        if (! head_)
            return nullptr;
        // Here we have at least one ready fiber. Unlink and return that.
        boost::fibers::fiber_context* f = head_;
        head_ = f->nxt;
        f->nxt = nullptr;

        std::cout << "pick_next() resuming " << properties(f).name << ": ";
        describe_ready_queue();
        return f;
    }

    virtual void property_change(boost::fibers::fiber_context* f, priority_props& props)
    {
        // Although our priority_props class defines multiple properties, only
        // one of them (priority) actually calls notify() when changed. The
        // point of a property_change() override is to reshuffle the ready
        // queue according to the updated priority value.
        std::cout << "property_change(" << props.name << '(' << props.get_priority()
                  << ")): ";

        // Despite the added complexity of the loop body, make a single pass
        // over the queue to find both the existing item and the new desired
        // insertion point.
        bool found = false;
        boost::fibers::fiber_context **insert = nullptr, **fp = &head_;
        for ( ; *fp; fp = &(*fp)->nxt)
        {
            if (*fp == f)
            {
                // found the passed fiber in our list -- unlink it
                found = true;
                *fp = (*fp)->nxt;
                f->nxt = nullptr;
                // If that was the last item in the list, stop.
                if (! *fp)
                    break;
                // If we've already found the new insertion point, no need to
                // continue looping.
                if (insert)
                    break;
            }
            // As in awakened(), we're looking for the first fiber in the
            // queue with priority lower than the passed fiber.
            if (properties(*fp).get_priority() < props.get_priority())
            {
                insert = fp;
                // If we've already found and unlinked the passed fiber, no
                // need to continue looping.
                if (found)
                    break;
            }
        }
        // property_change() should only be called if f->is_ready(). However,
        // a waiting fiber can change state to is_ready() while still on the
        // fiber_manager's waiting queue. Every such fiber will be swept onto
        // our ready queue before the next pick_next() call, but still it's
        // possible to get a property_change() call for a fiber that
        // is_ready() but is not yet on our ready queue. If it's not there, no
        // action required: we'll handle it next time it hits awakened().
        if (! found)
        {
            // hopefully user will distinguish this case by noticing that
            // the fiber with which we were called does not appear in the
            // ready queue at all
            describe_ready_queue();
            return;
        }
        // There might not be any ready fibers with lower priority. In that
        // case, append to the end of the queue.
        std::string where;
        if (insert)
            where = std::string("before ") + properties(*insert).name;
        else
        {
            insert = fp;
            where = "to end";
        }
        // Insert f at the new insertion point in the queue.
        f->nxt = *insert;
        *insert = f;

        std::cout << "moving " << where << ": ";
        describe_ready_queue();
    }

    void describe_ready_queue()
    {
        if (! head_)
            std::cout << "[empty]";
        else
        {
            const char* delim = "";
            for (boost::fibers::fiber_context *f = head_; f; f = f->nxt)
            {
                priority_props& props(properties(f));
                std::cout << delim << props.name << '(' << props.get_priority() << ')';
                delim = ", ";
            }
        }
        std::cout << std::endl;
    }
};

void init(const std::string& name, int priority)
{
    priority_props& props(boost::this_fiber::properties<priority_props>());
    props.name = name;
    props.set_priority(priority);
}

void yield_fn(const std::string& name, int priority)
{
    init(name, priority);

    for (int i = 0; i < 3; ++i)
    {
        std::cout << "fiber " << name << " running" << std::endl;
        boost::this_fiber::yield();
    }
}

void barrier_fn(const std::string& name, int priority, boost::fibers::barrier& barrier)
{
    init(name, priority);

    std::cout << "fiber " << name << " waiting on barrier" << std::endl;
    barrier.wait();
    std::cout << "fiber " << name << " yielding" << std::endl;
    boost::this_fiber::yield();
    std::cout << "fiber " << name << " done" << std::endl;
}

void change_fn(const std::string& name, int priority,
               boost::fibers::fiber& other, int other_priority,
               boost::fibers::barrier& barrier)
{
    init(name, priority);

    std::cout << "fiber " << name << " waiting on barrier" << std::endl;
    barrier.wait();
    // We assume a couple things about 'other':
    // - that it was also waiting on the same barrier
    // - that it has lower priority than this fiber.
    // If both are true, 'other' is now ready to run but is sitting in
    // priority_scheduler's ready queue. Change its priority.
    priority_props& other_props(other.properties<priority_props>());
    std::cout << "fiber " << name << " changing priority of " << other_props.name
              << " to " << other_priority << std::endl;
    other_props.set_priority(other_priority);
    std::cout << "fiber " << name << " done" << std::endl;
}

int main(int argc, char *argv[])
{
    // make sure we use our priority_scheduler rather than default round_robin
    priority_scheduler psched;
    boost::fibers::set_scheduling_algorithm(&psched);

    {
        // verify that high-priority fiber always gets scheduled first
        boost::fibers::fiber low(boost::bind(yield_fn, "low", 1));
        boost::fibers::fiber med(boost::bind(yield_fn, "medium", 2));
        boost::fibers::fiber hi(boost::bind(yield_fn, "high", 3));
        hi.join();
        med.join();
        low.join();
        std::cout << std::endl;
    }

    {
        // fibers of same priority are scheduled in round-robin order
        boost::fibers::fiber a(boost::bind(yield_fn, "a", 0));
        boost::fibers::fiber b(boost::bind(yield_fn, "b", 0));
        boost::fibers::fiber c(boost::bind(yield_fn, "c", 0));
        a.join();
        b.join();
        c.join();
        std::cout << std::endl;
    }

    {
        // using a barrier wakes up all waiting fibers at the same time
        boost::fibers::barrier barrier(3);
        boost::fibers::fiber low(boost::bind(barrier_fn, "low", 1, boost::ref(barrier)));
        boost::fibers::fiber med(boost::bind(barrier_fn, "medium", 2, boost::ref(barrier)));
        boost::fibers::fiber hi(boost::bind(barrier_fn, "high", 3, boost::ref(barrier)));
        low.join();
        med.join();
        hi.join();
        std::cout << std::endl;
    }

    {
        // change priority of a fiber in priority_scheduler's ready queue
        boost::fibers::barrier barrier(3);
        boost::fibers::fiber c(boost::bind(barrier_fn, "c", 1, boost::ref(barrier)));
        boost::fibers::fiber a(boost::bind(change_fn, "a", 3,
                                           boost::ref(c), 3, boost::ref(barrier)));
        boost::fibers::fiber b(boost::bind(barrier_fn, "b", 2, boost::ref(barrier)));
        a.join();
        b.join();
        c.join();
        std::cout << std::endl;
    }

    std::cout << "done." << std::endl;

    return EXIT_SUCCESS;
}
