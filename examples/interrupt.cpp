#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/bind.hpp>

#include <boost/fiber/all.hpp>

struct condition_test_data
{
    condition_test_data() : notified(0), awoken(0) { }

    boost::fibers::mutex mutex;
    boost::fibers::condition condition;
    int notified;
    int awoken;
};

void condition_test_fiber(condition_test_data* data)
{
    boost::unique_lock<boost::fibers::mutex> lock(data->mutex);
    while (!(data->notified > 0))
        data->condition.wait(lock);
    data->awoken++;
}

int main()
{
    boost::fibers::round_robin ds;
    boost::fibers::set_scheduling_algorithm( & ds);

    condition_test_data data;
    boost::fibers::fiber f(boost::bind(&condition_test_fiber, &data));

    f.interrupt();
    try
    {
        f.join();

        std::cout << "done." << std::endl;

        return EXIT_SUCCESS;
    }
    catch ( boost::fibers::fiber_interrupted const&)
    { std::cerr << "interrupted" << std::endl; }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }

    return EXIT_FAILURE;
}
