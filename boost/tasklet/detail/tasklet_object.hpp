
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKLETS_DETAIL_TASKLET_OBJECT_H
#define BOOST_TASKLETS_DETAIL_TASKLET_OBJECT_H

#include <cstddef>

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <boost/tasklet/detail/config.hpp>
#include <boost/tasklet/detail/tasklet_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace tasklets {
namespace detail {

template< typename Fn >
class tasklet_object : public tasklet_base
{
private:
	Fn	fn_;

	tasklet_object( tasklet_object &);
	tasklet_object & operator=( tasklet_object const&);

public:
	template< typename AllocatorT >
	tasklet_object( Fn fn, std::size_t stacksize, AllocatorT const& alloc) :
		tasklet_base( stacksize, alloc),
		fn_( fn)
	{}

	template< typename AllocatorT >
	tasklet_object( BOOST_RV_REF( Fn) fn, std::size_t stacksize, AllocatorT const& alloc) :
		tasklet_base( stacksize, alloc),
		fn_( fn)
	{}

	void exec()
	{ fn_(); }
};

template< typename Fn >
class tasklet_object< reference_wrapper< Fn > > : public tasklet_base
{
private:
	Fn	&	fn_;

	tasklet_object( tasklet_object &);
	tasklet_object & operator=( tasklet_object const&);

public:
	template< typename AllocatorT >
	tasklet_object( reference_wrapper< Fn > fn, std::size_t stacksize, AllocatorT const& alloc) :
		tasklet_base( stacksize, alloc),
		fn_( fn)
	{}

	void exec()
	{ fn_(); }
};

template< typename Fn >
class tasklet_object< const reference_wrapper< Fn > > : public tasklet_base
{
private:
	Fn	&	fn_;

	tasklet_object( tasklet_object &);
	tasklet_object & operator=( tasklet_object const&);

public:
	template< typename AllocatorT >
	tasklet_object( const reference_wrapper< Fn > fn, std::size_t stacksize, AllocatorT const& alloc) :
		tasklet_base( stacksize, alloc),
		fn_( fn)
	{}

	void exec()
	{ fn_(); }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_DETAIL_TASKLET_OBJECT_H
