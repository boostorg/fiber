
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_DETAIL_BIND_PROCESSOR_HPUX_H
#define BOOST_TASKS_DETAIL_BIND_PROCESSOR_HPUX_H

extern "C"
{
#include <sys/pthread.h>
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

	::pthread_spu_t spu;
	int errno_(
		::pthread_processor_bind_np(
			PTHREAD_BIND_FORCED_NP,
			& spu,
			static_cast< pthread_spu_t >( n),
			PTHREAD_SELFTID_NP) );
	if ( errno_ != 0)
		throw boost::system::system_error(
				boost::system::error_code(
					errno_,
					boost::system::system_category() ) );
}

inline
void bind_to_any_processor()
{
	::pthread_spu_t spu;
	int errno_(
		::pthread_processor_bind_np(
			PTHREAD_BIND_FORCED_NP,
			& spu,
			PTHREAD_SPUFLOAT_NP,
			PTHREAD_SELFTID_NP) );
	if ( errno_ != 0)
		throw boost::system::system_error(
				boost::system::error_code(
					errno_,
					boost::system::system_category() ) );
}

}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_DETAIL_BIND_PROCESSOR_HPUX_H
