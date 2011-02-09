
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/task/spin/barrier.hpp"

#include <stdexcept>

namespace boost {
namespace tasks {
namespace spin {

barrier::barrier( std::size_t initial) :
	initial_( initial),
	current_( initial_),
	cycle_( true),
	mtx_(),
	cond_()
{ if ( initial == 0) throw std::invalid_argument("invalid barrier count"); }

bool
barrier::wait()
{
	mutex::scoped_lock lk( mtx_);
	bool cycle( cycle_);
	if ( 0 == --current_)
	{
		cycle_ = ! cycle_;
		current_ = initial_;
		cond_.notify_all();
		return true;
	}
	else
	{
		while ( cycle == cycle_)
			cond_.wait( lk);
	}
	return false;
}

}}}
