
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/task/semaphore.hpp"

extern "C" {

#include <sys/stat.h>

}

#include <cerrno>
#include <stdexcept>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

namespace {

#if ! defined(__FreeBSD__)
union semun {
	int				val;
	semid_ds	*	buf;
	ushort		*	array;
};
#endif

}

namespace boost {
namespace tasks {

semaphore::semaphore( int sem_count) :
	handle_( -1)
{
	if ( 0 > sem_count)
		throw std::invalid_argument("count must not be negative");

	semun ctl;
	ctl.val = sem_count;
	BOOST_ASSERT( ctl.val == sem_count);

	if ( ( handle_ = ::semget( IPC_PRIVATE, 1, S_IRUSR | S_IWUSR) ) == -1)
		throw system::system_error( errno, system::system_category() );

	if ( ::semctl( handle_, 0, SETVAL, ctl) == -1) 
		throw system::system_error( errno, system::system_category() );
}

semaphore::~semaphore()
{ ::semctl( handle_, 0, IPC_RMID); }

void
semaphore::post( int n)
{
	 if ( 0 >= n)
		throw std::invalid_argument("invalid post-argument");

	sembuf op;

	op.sem_num = 0;
	op.sem_op = n;
	op.sem_flg = 0;
	BOOST_ASSERT( op.sem_op == n);

	if ( ::semop( handle_, & op, 1) == -1)
		throw system::system_error( errno, system::system_category() );
}

void
semaphore::wait()
{
	sembuf op;

	op.sem_num = 0;
	op.sem_op = -1;
	op.sem_flg = 0;

	if ( ::semop( handle_, & op, 1) == -1) 
		throw system::system_error( errno, system::system_category() );
}

bool 
semaphore::try_wait()
{
	sembuf op;

	op.sem_num = 0;
	op.sem_op = -1;
	op.sem_flg = IPC_NOWAIT; // Don't wait if we can't lock it now

	return ::semop( handle_, & op, 1) == 0; 
}

}}
