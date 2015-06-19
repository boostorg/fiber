
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/barrier.hpp"

#include <system_error>

#include "boost/fiber/exceptions.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

barrier::barrier( std::size_t initial) :
	initial_( initial),
	current_( initial_),
	cycle_( true),
	mtx_(),
	cond_() {
    if ( 0 == initial) {
        throw invalid_argument( static_cast< int >( std::errc::invalid_argument),
                                "boost fiber: zero initial barrier count");
    }
}

bool
barrier::wait() {
	std::unique_lock< mutex > lk( mtx_);
	bool cycle( cycle_);
	if ( 0 == --current_) {
		cycle_ = ! cycle_;
		current_ = initial_;
		cond_.notify_all();
		return true;
	} else {
		while ( cycle == cycle_) {
			cond_.wait( lk);
        }
	}
	return false;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
