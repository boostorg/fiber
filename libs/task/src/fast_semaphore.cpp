
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/task/fast_semaphore.hpp"

#include <limits>
#include <stdexcept>

namespace boost {
namespace tasks {

fast_semaphore::fast_semaphore( int sem_count, unsigned int spin_count) :
	spin_count_( spin_count),
	sem_count_( sem_count),
	sem_active_( true),
	sem_( sem_count)
{
	if ( 0 > sem_count_)
		throw std::invalid_argument("count must not be negative");
}

fast_semaphore::~fast_semaphore()
{}

void
fast_semaphore::post( int count)
{
	if ( 0 > count)
		throw std::invalid_argument("count must not be negative");

	int sem_count = sem_count_.fetch_add( count);

	if ( sem_count < 0)
		sem_.post( ( std::min)( count, -sem_count) );
}

void
fast_semaphore::wait()
{
	unsigned int spin_count( spin_count_);
	while ( 0 < spin_count--)
		if ( try_wait() ) return;

	if ( 0 <= sem_count_.fetch_sub( 1) ) return;

	if ( ! sem_active_.load() ) return;

	sem_.wait();
}

bool 
fast_semaphore::try_wait()
{
	int sem_count;

	do
	{
		sem_count = sem_count_.load();
		if ( ! sem_active_.load() ) return false;
	} while ( sem_count > 0 && ! sem_count_.compare_exchange_strong( sem_count, sem_count - 1) );

	return sem_count > 0;
}

void
fast_semaphore::deactivate() {
	sem_active_.store( false);
	post( std::abs( sem_count_.load() ) );
}

}}
