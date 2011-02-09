
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef PERFORMANCE_GCC_I386_H
#define PERFORMANCE_GCC_I386_H

#include <algorithm>
#include <cstddef>
#include <vector>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>

typedef uint64_t cycle_t;

inline
cycle_t get_cycles()
{
	uint32_t res[2];

	__asm__ __volatile__ (
		"xorl %%eax, %%eax\n"
		"cpuid\n"
		::: "%eax", "%ebx", "%ecx", "%edx"
	);
	__asm__ __volatile__ ("rdtsc" : "=a" (res[0]), "=d" (res[1]) );
	__asm__ __volatile__ (
		"xorl %%eax, %%eax\n"
		"cpuid\n"
		::: "%eax", "%ebx", "%ecx", "%edx"
	);

	return * ( cycle_t *)res;
}

struct measure
{
	cycle_t operator()()
	{
		cycle_t start( get_cycles() );
		return get_cycles() - start;
	}
};

inline
cycle_t get_overhead()
{
	std::size_t iterations( 10);
	std::vector< cycle_t >	overhead( iterations, 0);
	for ( std::size_t i( 0); i < iterations; ++i)
		std::generate(
			overhead.begin(), overhead.end(),
			measure() );
	BOOST_ASSERT( overhead.begin() != overhead.end() );
	return * std::min_element( overhead.begin(), overhead.end() );
}

#endif // PERFORMANCE_GCC_I386_H
