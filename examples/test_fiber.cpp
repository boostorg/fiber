#include <string>

#include <boost/bind.hpp>

#include <boost/fiber/all.hpp>

void foo( std::string const& str, int n)
{
	for ( int i = 0; i < n; ++i)
	{
		std::cout << i << ": " << str << std::endl;
		boost::this_fiber::yield();
	}
}

void bar()
{
    boost::fibers::fiber f1( boost::bind( foo, "abc", 5) );
    boost::fibers::fiber f2( boost::bind( foo, "xyz", 7) );

    f1.join();
    f2.join();
}
