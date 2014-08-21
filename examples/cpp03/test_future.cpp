#include <string>

#include <boost/bind.hpp>

#include <boost/fiber/all.hpp>

int foo( std::string const& str, int n)
{
	for ( int i = 0; i < n; ++i)
	{
		std::cout << i << ": " << str << std::endl;
		boost::this_fiber::yield();
	}

    return n;
}

void bar()
{
    boost::fibers::future< int > fi(
        boost::fibers::async(
            boost::bind( foo, "abc", 5) ) );
    fi.wait();
}
