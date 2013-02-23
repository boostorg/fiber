
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_spin::mutex

#ifndef BOOST_FIBERS_SPINLOCK_H
#define BOOST_FIBERS_SPINLOCK_H

#include <boost/atomic.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>

namespace boost {
namespace fibers {
namespace detail {

class BOOST_FIBERS_DECL spinlock : private noncopyable
{
private:
	static const int LOCKED;
	static const int UNLOCKED;

	atomic< int >   state_;

public:
	spinlock();

	void lock();

	void unlock();
};

}}}

#endif // BOOST_FIBERS_SPINLOCK_H
