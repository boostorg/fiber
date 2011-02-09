
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_DETAIL_BIND_PROCESSOR_WINDOWS_H
#define BOOST_TASKS_DETAIL_BIND_PROCESSOR_WINDOWS_H

extern "C"
{
#include <windows.h>
}

#include <boost/assert.hpp>
#include <boost/thread.hpp>
#include <boost/system/system_error.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace this_thread {

inline
void bind_to_processor( unsigned int n)
{
	BOOST_ASSERT( n >= 0);
	BOOST_ASSERT( n < boost::thread::hardware_concurrency() );

	if ( ::SetThreadAffinityMask( ::GetCurrentThread(), ( DWORD_PTR)1 << n) == 0)
		throw boost::system::system_error(
				boost::system::error_code(
					::GetLastError(),
					boost::system::system_category() ) );
}

inline
void bind_to_any_processor()
{
	DWORD_PTR ptr( 1);
	for ( unsigned int i( 0); i < boost::thread::hardware_concurrency(); ++i)
		ptr = ptr << i;

	if ( ::SetThreadAffinityMask( ::GetCurrentThread(), ptr) == 0)
		throw boost::system::system_error(
				boost::system::error_code(
					::GetLastError(),
					boost::system::system_category() ) );
}

}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_DETAIL_BIND_PROCESSOR_WINDOWS_H
