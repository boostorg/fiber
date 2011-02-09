
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_DETAIL_WSQ_H
#define BOOST_TASKS_DETAIL_WSQ_H

#include <boost/atomic.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/utility.hpp>

#include <boost/task/callable.hpp>
#include <boost/task/detail/config.hpp>
#include <boost/task/fast_semaphore.hpp>

#include <boost/config/abi_prefix.hpp>

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace tasks {
namespace detail {

class BOOST_TASKS_DECL wsq : private noncopyable
{
private:
	const int					initial_size_;
	shared_array< callable >	array_;
	int							capacity_;
	int							mask_;
	atomic< unsigned int >		head_idx_;
	atomic< unsigned int >		tail_idx_;
	recursive_mutex				mtx_;
	fast_semaphore			&	fsem_;

public:
	wsq( fast_semaphore & fsem);

	bool empty() const;

	std::size_t size() const;

	void put( callable const&);

	bool try_take( callable &);

	bool try_steal( callable &);
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_DETAIL_WSQ_H

