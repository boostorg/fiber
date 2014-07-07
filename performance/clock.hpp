
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef CLOCK_H
#define CLOCK_H

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

#include <boost/assert.hpp>
#include <boost/chrono.hpp>
#include <boost/cstdint.hpp>

typedef boost::chrono::high_resolution_clock    clock_type;
typedef chronoduration                    duration_type;
typedef chronotime_point                  time_point_type;

struct clock_overhead
{
    boost::uint64_t operator()()
    {
        time_point_type start( chrononow() );
        return ( chrononow() - start).count();
    }
};

duration_type overhead_clock()
{
    std::size_t iterations( 10);
    std::vector< boost::uint64_t >  overhead( iterations, 0);
    for ( std::size_t i = 0; i < iterations; ++i)
        std::generate(
            overhead.begin(), overhead.end(),
            clock_overhead() );
    BOOST_ASSERT( overhead.begin() != overhead.end() );
    return duration_type( std::accumulate( overhead.begin(), overhead.end(), 0) / iterations);
}

#endif // CLOCK_H
