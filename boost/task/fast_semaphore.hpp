
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASK_FAST_SEMAPHORE_H
#define BOOST_TASK_FAST_SEMAPHORE_H

#include <boost/task/detail/config.hpp>

#include <boost/atomic.hpp>
#include <boost/utility.hpp>

#include <boost/task/semaphore.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {

class BOOST_TASKS_DECL fast_semaphore : private boost::noncopyable
{
private:
	unsigned int	spin_count_;
	atomic< int >	sem_count_;
	atomic< bool >	sem_active_;
	semaphore		sem_;

public:
	fast_semaphore( int, unsigned int = 0);

	~fast_semaphore();

	void post( int = 1);

	void wait();

	bool try_wait();

	void deactivate();
};

}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASK_FAST_SEMAPHORE_H
