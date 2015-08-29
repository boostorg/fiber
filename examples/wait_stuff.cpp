//          Copyright Nat Goodspeed 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/all.hpp>
#include <boost/noncopyable.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>
#include <type_traits>              // std::result_of
#include <iostream>
#include <sstream>
#include <memory>                   // std::shared_ptr
#include <chrono>
#include <string>
#include <cassert>

// These are wait_something() functions rather than when_something()
// functions. A big part of the point of the Fiber library is to model
// sequencing using the processor's instruction pointer rather than chains of
// callbacks. The future-oriented when_all() / when_any() functions are still
// based on chains of callbacks. With Fiber, we can do better.

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
*   Verbose
*****************************************************************************/
class Verbose: boost::noncopyable
{
public:
    Verbose(const std::string& d):
        desc(d)
    {
        std::cout << desc << " start" << std::endl;
    }

    ~Verbose()
    {
        std::cout << desc << " stop" << std::endl;
    }

private:
    const std::string desc;
};

/*****************************************************************************
*   when_any, simple completion
*****************************************************************************/
// Degenerate case: when there are no functions to wait for, return
// immediately.
void wait_first_simple_impl(Done::ptr)
{}

// When there's at least one function to wait for, launch it and recur to
// process the rest.
template <typename Fn, typename... Fns>
void wait_first_simple_impl(Done::ptr done, Fn && function, Fns&& ... functions)
{
    boost::fibers::fiber([done, function](){
        function();
        done->notify();
    }).detach();
    wait_first_simple_impl(done, std::forward<Fns>(functions)...);
}

// interface function: instantiate Done, launch tasks, wait for Done
template < typename... Fns >
void wait_first_simple(Fns&& ... functions)
{
    // Use shared_ptr because each function's fiber will bind it separately,
    // and we're going to return before the last of them completes.
    auto done(std::make_shared<Done>());
    wait_first_simple_impl(done, std::forward<Fns>(functions)...);
    done->wait();
}

/*****************************************************************************
*   when_any, return value
*****************************************************************************/
// When there's only one function, call this overload
template < typename T, typename Fn >
void wait_first_value_impl(std::shared_ptr<boost::fibers::bounded_channel<T>> channel,
                           Fn && function)
{
    boost::fibers::fiber([channel, function](){
        // Ignore channel_op_status returned by push(): might be closed, might
        // be full; we simply don't care.
        channel->push(function());
    }).detach();
}

// When there are two or more functions, call this overload
template < typename T, typename Fn0, typename Fn1, typename... Fns >
void wait_first_value_impl(std::shared_ptr<boost::fibers::bounded_channel<T>> channel,
                           Fn0 && function0,
                           Fn1 && function1,
                           Fns && ... functions)
{
    // process the first function using the single-function overload
    wait_first_value_impl<T>(channel, function0);
    // then recur to process the rest
    wait_first_value_impl<T>(channel, std::forward<Fn1>(function1),
                             std::forward<Fns>(functions)...);
}

// Assume that all passed functions have the same return type. The return type
// of wait_first_value() is the return type of the first passed function. It is
// simply invalid to pass NO functions.
template < typename Fn, typename... Fns >
typename std::result_of<Fn()>::type
wait_first_value(Fn && function, Fns && ... functions)
{
    typedef typename std::result_of<Fn()>::type return_t;
    typedef boost::fibers::bounded_channel<return_t> channel_t;
    // bounded_channel of size 1: only store the first value
    auto channelp(std::make_shared<channel_t>(1));
    // launch all the relevant fibers
    wait_first_value_impl<return_t>(channelp, std::forward<Fn>(function),
                                    std::forward<Fns>(functions)...);
    // retrieve the first value
    return_t value(channelp->value_pop());
    // close the channel: no subsequent push() has to succeed
    channelp->close();
    return value;
}

/*****************************************************************************
*   when_any, produce first outcome, whether result or exception
*****************************************************************************/
// When there's only one function, call this overload.
template < typename T, typename Fn >
void wait_first_outcome_impl(std::shared_ptr<boost::fibers::bounded_channel<boost::fibers::future<T>>> channel,
                             Fn && function)
{
    boost::fibers::fiber([channel, function](){
        // Instantiate a packaged_task to capture any exception thrown by
        // function.
        boost::fibers::packaged_task<T()> task(function);
        // Immediately run this packaged_task on same fiber. We want
        // function() to have completed BEFORE we push the future.
        task();
        // Pass the corresponding future to consumer. Ignore channel_op_status
        // returned by push(): might be closed, might be full; we simply don't
        // care.
        channel->push(task.get_future());
    }).detach();
}

// When there are two or more functions, call this overload
template < typename T, typename Fn0, typename Fn1, typename... Fns >
void wait_first_outcome_impl(std::shared_ptr<boost::fibers::bounded_channel<boost::fibers::future<T>>> channel,
                             Fn0 && function0,
                         Fn1 && function1,
                         Fns && ... functions)
{
    // process the first function using the single-function overload
    wait_first_outcome_impl<T>(channel, function0);
    // then recur to process the rest
    wait_first_outcome_impl<T>(channel, std::forward<Fn1>(function1),
                               std::forward<Fns>(functions)...);
}

// Assume that all passed functions have the same return type. The return type
// of wait_first_outcome() is the return type of the first passed function. It is
// simply invalid to pass NO functions.
template < typename Fn, typename... Fns >
typename std::result_of<Fn()>::type
wait_first_outcome(Fn && function, Fns && ... functions)
{
    // In this case, the value we pass through the channel is actually a
    // future -- which is already ready. future can carry either a value or an
    // exception.
    typedef typename std::result_of<Fn()>::type return_t;
    typedef boost::fibers::future<return_t> future_t;
    typedef boost::fibers::bounded_channel<future_t> channel_t;
    // bounded_channel of size 1: only store the first future
    auto channelp(std::make_shared<channel_t>(1));
    // launch all the relevant fibers
    wait_first_outcome_impl<return_t>(channelp, std::forward<Fn>(function),
                                      std::forward<Fns>(functions)...);
    // retrieve the first future
    future_t future(channelp->value_pop());
    // close the channel: no subsequent push() has to succeed
    channelp->close();
    // either return value or throw exception
    return future.get();
}

/*****************************************************************************
*   when_any, collect exceptions until success; throw exception_list if no
*   success
*****************************************************************************/
// define an exception to aggregate exception_ptrs; prefer
// std::exception_list (N4407 et al.) once that becomes available
class exception_list: public std::runtime_error
{
public:
    exception_list(const std::string& what):
        std::runtime_error(what)
    {}

    typedef std::vector<std::exception_ptr> bundle_t;

    // N4407 proposed std::exception_list API
    typedef bundle_t::const_iterator iterator;
    std::size_t size()  const noexcept { return bundle_.size(); }
    iterator    begin() const noexcept { return bundle_.begin(); }
    iterator    end()   const noexcept { return bundle_.end(); }

    // extension to populate
    void add(std::exception_ptr ep) { bundle_.push_back(ep); }

private:
    bundle_t bundle_;
};

// Assume that all passed functions have the same return type. The return type
// of wait_first_success() is the return type of the first passed function. It is
// simply invalid to pass NO functions.
template < typename Fn, typename... Fns >
typename std::result_of<Fn()>::type
wait_first_success(Fn && function, Fns && ... functions)
{
    std::size_t count(1 + sizeof...(functions));
    // In this case, the value we pass through the channel is actually a
    // future -- which is already ready. future can carry either a value or an
    // exception.
    typedef typename std::result_of<Fn()>::type return_t;
    typedef boost::fibers::future<return_t> future_t;
    typedef boost::fibers::bounded_channel<future_t> channel_t;
    // make bounded_channel big enough to hold all results if need be
    // (could use unbounded_channel this time, but let's just share
    // wait_first_outcome_impl())
    auto channelp(std::make_shared<channel_t>(count));
    // launch all the relevant fibers
    wait_first_outcome_impl<return_t>(channelp, std::forward<Fn>(function),
                                      std::forward<Fns>(functions)...);
    // instantiate exception_list, just in case
    exception_list exceptions("wait_first_success() produced only errors");
    // retrieve up to 'count' results -- but stop there!
    for (std::size_t i = 0; i < count; ++i)
    {
        // retrieve the next future
        future_t future(channelp->value_pop());
        // retrieve exception_ptr if any
        std::exception_ptr error(future.get_exception_ptr());
        // if no error, then yay, return value
        if (! error)
        {
            // close the channel: no subsequent push() has to succeed
            channelp->close();
            // show caller the value we got
            return future.get();
        }

        // error is non-null: collect
        exceptions.add(error);
    }
    // We only arrive here when every passed function threw an exception.
    // Throw our collection to inform caller.
    throw exceptions;
}

/*****************************************************************************
*   when_any, heterogeneous
*****************************************************************************/
// No need to break out the first Fn for interface function: let the compiler
// complain if empty.
// Our functions have different return types, and we might have to return any
// of them. Use a variant, expanding std::result_of<Fn()>::type for each Fn in
// parameter pack.
template < typename... Fns >
boost::variant< typename std::result_of<Fns()>::type... >
wait_first_value_het(Fns && ... functions)
{
    // Use bounded_channel<boost::variant<T1, T2, ...>>; see remarks above.
    typedef boost::variant< typename std::result_of<Fns()>::type... > return_t;
    typedef boost::fibers::bounded_channel<return_t> channel_t;
    // bounded_channel of size 1: only store the first value
    auto channelp(std::make_shared<channel_t>(1));
    // launch all the relevant fibers
    wait_first_value_impl<return_t>(channelp, std::forward<Fns>(functions)...);
    // retrieve the first value
    return_t value(channelp->value_pop());
    // close the channel: no subsequent push() has to succeed
    channelp->close();
    return value;
}

/*****************************************************************************
*   when_all, simple completion
*****************************************************************************/
// interface function: instantiate barrier, launch tasks, wait for barrier
template < typename... Fns >
void wait_all_simple(Fns&& ... functions)
{
    std::size_t count(sizeof...(functions));
    // Initialize a barrier(count+1) because we'll immediately wait on it. We
    // don't want to wake up until 'count' more fibers wait on it. Even though
    // we'll stick around until the last of them completes, use shared_ptr
    // anyway so we can reuse wait_first_simple_impl().
    auto barrier(std::make_shared<boost::fibers::barrier>(count+1));
    wait_first_simple_impl(barrier, std::forward<Fns>(functions)...);
    barrier->wait();
}

/*****************************************************************************
*   when_all, return values
*****************************************************************************/
// wait_all_source() returns shared_ptr<unbounded_channel<T>>. wait_all()
// populates and returns vector<T> by looping over channel until closed. BUT
// -- real code might prefer to consume each result as soon as available.

/*****************************************************************************
*   when_all, throw first exception
*****************************************************************************/
// wait_all_source() returns shared_ptr<unbounded_channel<future<T>>>.
// If wait_all() just calls get(), first exception propagates.

/*****************************************************************************
*   when_all, collect exceptions
*****************************************************************************/
// Like the previous, but introduce 'exceptions', a std::runtime_error
// subclass with a vector of std::exception_ptr. wait_all() collects
// exception_ptrs and throws if non-empty; else returns vector<T>.

/*****************************************************************************
*   when_all, heterogeneous
*****************************************************************************/
// Accept a struct template argument, return that struct! Use initializer list
// populated from arg pack to populate. Assuming unequal types, there's no
// point in processing the first result to arrive: results aren't
// interchangeable, each must go in its own slot. First exception propagates.

/*****************************************************************************
*   example task functions
*****************************************************************************/
template <typename T>
T sleeper_impl(T item, int ms, bool thrw=false)
{
    std::ostringstream descb, funcb;
    descb << item;
    std::string desc(descb.str());
    funcb << "  sleeper(" << item << ")";
    Verbose v(funcb.str());

    boost::this_fiber::sleep_for(std::chrono::milliseconds(ms));
    if (thrw)
        throw std::runtime_error(desc);
    return item;
}

inline
std::string sleeper(const std::string& item, int ms, bool thrw=false)
{
    return sleeper_impl(item, ms, thrw);
}

inline
double sleeper(double item, int ms, bool thrw=false)
{
    return sleeper_impl(item, ms, thrw);
}

inline
int sleeper(int item, int ms, bool thrw=false)
{
    return sleeper_impl(item, ms, thrw);
}

/*****************************************************************************
*   driving logic
*****************************************************************************/
int main( int argc, char *argv[]) {
    /*-------------------------- wait_first_simple ---------------------------*/
    {
        Verbose v("wait_first_simple()");
        wait_first_simple([](){ sleeper("wfs_long",   150); },
                          [](){ sleeper("wfs_medium", 100); },
                          [](){ sleeper("wfs_short",   50); });
    }

    //=> What happens to exception in detached fiber?
    //=> What happens when consumer calls get() (unblocking one producer) and then
    // calls close()?

    /*--------------------------- wait_first_value ---------------------------*/
    {
        Verbose v("wait_first_value()");
        std::string result =
            wait_first_value([](){ return sleeper("wfv_third",  150); },
                             [](){ return sleeper("wfv_second", 100); },
                             [](){ return sleeper("wfv_first",   50); });
        std::cout << "wait_first_value() => " << result << std::endl;
        assert(result == "wfv_first");
    }

    /*------------------------- wait_first_outcome -------------------------*/
    {
        Verbose v("wait_first_outcome()");
        std::string result =
            wait_first_outcome([](){ return sleeper("wfos_first",   50); },
                               [](){ return sleeper("wfos_second", 100); },
                               [](){ return sleeper("wfos_third",  150); });
        std::cout << "wait_first_outcome(success) => " << result << std::endl;
        assert(result == "wfos_first");

        std::string thrown;
        try
        {
            result =
                wait_first_outcome([](){ return sleeper("wfof_first",   50, true); },
                                   [](){ return sleeper("wfof_second", 100); },
                                   [](){ return sleeper("wfof_third",  150); });
        }
        catch (const std::exception& e)
        {
            thrown = e.what();
        }
        std::cout << "wait_first_outcome(fail) threw '" << thrown << "'" << std::endl;
        assert(thrown == "wfof_first");
    }

    /*------------------------- wait_first_success -------------------------*/
    {
        Verbose v("wait_first_success()");
        std::string result =
            wait_first_success([](){ return sleeper("wfss_first",   50, true); },
                               [](){ return sleeper("wfss_second", 100); },
                               [](){ return sleeper("wfss_third",  150); });
        std::cout << "wait_first_success(success) => " << result << std::endl;
        assert(result == "wfss_second");

        std::string thrown;
        std::size_t count = 0;
        try
        {
            result =
                wait_first_success([](){ return sleeper("wfsf_first",   50, true); },
                                   [](){ return sleeper("wfsf_second", 100, true); },
                                   [](){ return sleeper("wfsf_third",  150, true); });
        }
        catch (const exception_list& e)
        {
            thrown = e.what();
            count = e.size();
        }
        catch (const std::exception& e)
        {
            thrown = e.what();
        }
        std::cout << "wait_first_success(fail) threw '" << thrown << "': "
                  << count << " errors" << std::endl;
        assert(thrown == "wait_first_success() produced only errors");
        assert(count == 3);
    }

    /*------------------------ wait_first_value_het ------------------------*/
    {
        Verbose v("wait_first_value_het()");
        boost::variant<std::string, double, int> result =
            wait_first_value_het([](){ return sleeper("wfvh_third",  150); },
                                 [](){ return sleeper(3.14,          100); },
                                 [](){ return sleeper(17,             50); });
        std::cout << "wait_first_value_het() => " << result << std::endl;
        assert(boost::get<int>(result) == 17);
    }

    /*-------------------------- wait_all_simple ---------------------------*/
    {
        Verbose v("wait_all_simple()");
        wait_all_simple([](){ sleeper("was_long",   150); },
                        [](){ sleeper("was_medium", 100); },
                        [](){ sleeper("was_short",   50); });
    }

    return EXIT_SUCCESS;
}
