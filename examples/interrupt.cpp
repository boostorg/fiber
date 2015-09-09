#include <cstdlib>
#include <iostream>
#include <string>

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
    std::unique_lock<boost::fibers::mutex> lock(data->mutex);
    while (!(data->notified > 0))
        data->condition.wait(lock);
    data->awoken++;
}

int main()
{
    condition_test_data data;
    boost::fibers::fiber f( & condition_test_fiber, & data);

    f.interrupt();
    try
    {
        f.join();
    }
    catch ( boost::fibers::fiber_interrupted const&)
    { std::cerr << "interrupted" << std::endl; }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }

    std::cout << "done." << std::endl;

    return EXIT_SUCCESS;
}
