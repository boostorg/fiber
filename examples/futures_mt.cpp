
//  (C) Copyright 2013 Oliver Kowalke
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <boost/assert.hpp>
#include <boost/fiber/all.hpp>

template< typename Fn >
class rref {
public:
    rref( Fn && fn) :
        fn_( std::forward< Fn >( fn) ) {
    }

    rref( rref & other) :
        fn_( std::forward< Fn >( other.fn_) ) {
    }

    rref( rref && other) :
        fn_( std::forward< Fn >( other.fn_) ) {
    }

    rref( rref const& other) = delete;
    rref & operator=( rref const& other) = delete;

    void operator()() {
        return fn_();
    }

private:
    Fn  fn_;
};

int fn( int i)
{ return i; }

boost::fibers::future< int > async( int i)
{
    typedef boost::fibers::packaged_task< int() > packaged_task_t;
    packaged_task_t pt( std::bind( fn, i) );
    boost::fibers::future< int > f( pt.get_future() );
    typedef rref< packaged_task_t > rref_t;
    rref_t rr( std::move( pt) );
    std::thread( [=] () mutable { boost::fibers::fiber( rr).join(); } ).detach();
    return std::move( f);
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
