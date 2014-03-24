//  (C) Copyright 2013 Oliver Kowalke
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <utility>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/fiber/all.hpp>
#include <boost/move/move.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

typedef boost::shared_ptr< boost::fibers::packaged_task< int() > >  packaged_task_t;

int fn( int i)
{ return i; }

void exec( packaged_task_t pt)
{
    boost::fibers::fiber( boost::move( * pt) ).join();
}

boost::fibers::future< int > async( int i)
{
    packaged_task_t pt(
        new boost::fibers::packaged_task< int() >(
            boost::bind( fn, i) ) );
    boost::fibers::future< int > f( pt->get_future() );
    boost::thread( boost::bind( exec, pt) ).detach();
    return boost::move( f);
}

int main( int argc, char * argv[])
{
    for ( int i = 0; i < 5; ++i)
    {
        int n = 3;
        boost::fibers::future< int > f = async( n);
        int result = f.get();
        BOOST_ASSERT( n == result);
        std::cout << "result == " << result << std::endl;
    }

    return 0;
}
