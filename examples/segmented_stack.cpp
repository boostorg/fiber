
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/assert.hpp>
#include <boost/fiber/all.hpp>
#include <boost/thread.hpp>

int count = 20;
#if defined(BOOST_USE_SEGMENTED_STACKS)
void access( char *buf) __attribute__ ((noinline));
#endif
void access( char *buf)
{
  buf[0] = '\0';
}

void bar( int i)
{
    char buf[4 * 1024];

    if ( i > 0)
    {
        access( buf);
        std::cout << i << ". iteration" << std::endl;
        bar( i - 1);
    }
}

void foo()
{
    bar( count);
	boost::this_fiber::yield();
}

void thread_fn()
{
    boost::fibers::round_robin ds;
    boost::fibers::set_scheduling_algorithm( & ds);

    {
        boost::fibers::fiber f( foo);
        f.join();
    }
}

int main( int argc, char * argv[])
{
    std::cout << "using standard stacks: allocates " << count << " * 4kB on stack, ";
    std::cout << "initial stack size = " << boost::fibers::stack_allocator::default_stacksize() / 1024 << "kB" << std::endl;
#if defined(BOOST_USE_SEGMENTED_STACKS)
    std::cout << "application should not fail" << std::endl;
#else
    std::cout << "application might fail" << std::endl;
#endif

    boost::thread( thread_fn).join();

    return 0;
}
