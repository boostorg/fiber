#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/fiber/all.hpp>

struct X
{
    typedef boost::intrusive_ptr< X >     ptr_t;

    std::size_t use_count;

    X() : use_count( 0) {}

    friend inline void intrusive_ptr_add_ref( X * p) BOOST_NOEXCEPT
    { ++p->use_count; }

    friend inline void intrusive_ptr_release( X * p)
    { if ( 0 == --p->use_count) delete p; }

};

inline
void fn( std::string const& str, int n)
{
	for ( int i = 0; i < n; ++i)
	{
		std::cout << i << ": " << str << std::endl;
		boost::this_fiber::yield();
	}
}

int main()
{
    boost::fibers::round_robin ds;
    boost::fibers::set_scheduling_algorithm( & ds);

	try
	{
#if 0
        boost::fibers::fiber f1( boost::bind( fn, "abc", 5) );
        boost::fibers::fiber f2( boost::bind( fn, "xyz", 7) );

	f1.join();
	f2.join();
#endif

    X::ptr_t x1( new X() );
    X::ptr_t x2( new X() );
    std::cout << "x1->use-count: " << x1->use_count << std::endl;
    std::cout << "x2->use-count: " << x2->use_count << std::endl;

    intrusive_ptr_add_ref( x1.get() );
    intrusive_ptr_add_ref( x2.get() );
    std::cout << "x1->use-count: " << x1->use_count << std::endl;
    std::cout << "x2->use-count: " << x2->use_count << std::endl;

    X::ptr_t x3( 0);
    boost::atomic< X::ptr_t > av1( x1);
    boost::atomic< X::ptr_t > av2( x3);
    std::cout << "x1->use-count: " << x1->use_count << std::endl;
    std::cout << "x2->use-count: " << x2->use_count << std::endl;

    BOOST_ASSERT( av1.compare_exchange_strong( x1, x2) );
    std::cout << "x1->use-count: " << x1->use_count << std::endl;
    std::cout << "x2->use-count: " << x2->use_count << std::endl;
    //BOOST_ASSERT( av1.is_lock_free() );
    BOOST_ASSERT( av1.load() != x1);
    BOOST_ASSERT( av1.load() == x2);

    BOOST_ASSERT( av2.compare_exchange_strong( x3, x1) );
    std::cout << "x1->use-count: " << x1->use_count << std::endl;
    std::cout << "x2->use-count: " << x2->use_count << std::endl;
    BOOST_ASSERT( av2.load() == x1);
    BOOST_ASSERT( av2.load() != x2);

		std::cout << "done." << std::endl;

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
