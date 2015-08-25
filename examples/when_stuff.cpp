//          Copyright Nat Goodspeed 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/all.hpp>
#include <iostream>
#include <memory>                   // std::shared_ptr
#include <chrono>
#include <string>

/*****************************************************************************
*   Done
*****************************************************************************/
// Wrap canonical pattern for condition_variable + bool flag
struct Done
{
private:
    boost::fibers::condition_variable cond;
    boost::fibers::mutex mutex;
    bool ready = false;

public:
    typedef std::shared_ptr<Done> ptr;

    void wait()
    {
        std::unique_lock<boost::fibers::mutex> lock(mutex);
        while (! ready)
            cond.wait(lock);
    }

    void notify()
    {
        {
            std::unique_lock<boost::fibers::mutex> lock(mutex);
            ready = true;
        } // release mutex
        cond.notify_one();
    }
};

/*****************************************************************************
*   when_any, simple completion
*****************************************************************************/
// Degenerate case: when there are no functions to wait for, return
// immediately.
void when_any_simple_impl(Done::ptr)
{}

// When there's at least one function to wait for, launch it and recur to
// process the rest.
template <typename Fn, typename... Fns>
void when_any_simple_impl(Done::ptr done, Fn && function, Fns&& ... functions)
{
    boost::fibers::fiber([done, function](){
        function();
        done->notify();
    }).detach();
    when_any_simple_impl(done, std::forward<Fns>(functions)...);
}

// interface function: instantiate Done, launch tasks, wait for Done
template < typename... Fns >
void when_any_simple(Fns&& ... functions)
{
    // Use shared_ptr because each function's fiber will bind it separately,
    // and we're going to return before the last of them completes.
    auto done(std::make_shared<Done>());
    when_any_simple_impl(done, std::forward<Fns>(functions)...);
    done->wait();
}

/*****************************************************************************
*   example task functions
*****************************************************************************/
void sleeper(const std::string& desc, int ms)
{
    std::cout << "  " << desc << "() start" << std::endl;
    boost::this_fiber::sleep_for(std::chrono::milliseconds(ms));
    std::cout << "  " << desc << "() stop" << std::endl;
}

/*****************************************************************************
*   driving logic
*****************************************************************************/
int main( int argc, char *argv[]) {

    std::cout << "when_any_simple() start" << std::endl;
    when_any_simple([](){ sleeper("long",   150); },
                    [](){ sleeper("medium", 100); },
                    [](){ sleeper("short",   50); });
    std::cout << "when_any_simple() stop" << std::endl;

    // We've detached several fibers. We can't join() them; they're detached.
    // We have to detach them because the whole point of when_any_simple() is
    // to resume as soon as the FIRST one completes. But having those fibers
    // terminate after main() exits is a bit problematic. Just give them some
    // time to finish.
    boost::this_fiber::sleep_for(std::chrono::milliseconds(200));

    return EXIT_SUCCESS;
}
