
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKLETS_TASKLET_H
#define BOOST_TASKLETS_TASKLET_H

#include <cstddef>
#include <iostream>

#include <boost/config.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/move/move.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet/detail/config.hpp>
#include <boost/tasklet/detail/tasklet_base.hpp>
#include <boost/tasklet/detail/tasklet_object.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251)
# endif

namespace boost {
namespace tasklets {

class condition;
class mutex;
class strategy;
template< typename Strategy >
class scheduler;

class BOOST_TASKLET_DECL tasklet
{
private:
	friend class condition;
	friend class mutex;
	friend class strategy;
	template< typename Strategy >
	friend class scheduler;

	BOOST_COPYABLE_AND_MOVABLE( tasklet);

	detail::tasklet_base::ptr	impl_;

	template< typename Fn, typename AllocatorT >
	static detail::tasklet_base::ptr make_impl_(
		Fn fn, std::size_t stacksize, AllocatorT const& alloc)
	{
		return detail::tasklet_base::ptr(
			new detail::tasklet_object< Fn >( fn, stacksize, alloc) );
	}

	template< typename Fn, typename AllocatorT >
	static detail::tasklet_base::ptr make_impl_(
		BOOST_RV_REF( Fn) fn, std::size_t stacksize, AllocatorT const& alloc)
	{
		return detail::tasklet_base::ptr(
			new detail::tasklet_object< Fn >( fn, stacksize, alloc) );
	}

public:
	static std::size_t default_stacksize;

	class id;

	tasklet();

	template< typename Fn, typename AllocatorT >
	explicit tasklet( Fn fn, std::size_t stacksize, AllocatorT const& alloc) :
		impl_( make_impl_( fn, stacksize, alloc) )
	{}

#define BOOST_TASKLET_ARG(z, n, unused) \
   BOOST_PP_CAT(A, n) BOOST_PP_CAT(a, n)
#define BOOST_ENUM_TASKLET_ARGS(n) BOOST_PP_ENUM(n, BOOST_TASKLET_ARG, ~)

#define BOOST_TASKLET_TASKLET_CTOR(z, n, unused) \
	template< typename Fn, BOOST_PP_ENUM_PARAMS(n, typename A), typename AllocatorT > \
	tasklet( Fn fn, BOOST_ENUM_TASKLET_ARGS(n), std::size_t stacksize, AllocatorT const& alloc) : \
		impl_( \
			make_impl_( \
				boost::bind( boost::type< void >(), fn, BOOST_PP_ENUM_PARAMS(n, a) ), \
			   	stacksize, alloc) ) \
	{} \

#ifndef BOOST_TASKLET_MAX_ARITY
#define BOOST_TASKLET_MAX_ARITY 10
#endif

BOOST_PP_REPEAT_FROM_TO( 1, BOOST_TASKLET_MAX_ARITY, BOOST_TASKLET_TASKLET_CTOR, ~)

#undef BOOST_TASKLET_TASKLET_CTOR

	template< typename Fn, typename AllocatorT >
	explicit tasklet( BOOST_RV_REF( Fn) fn, std::size_t stacksize, AllocatorT const& alloc) :
		impl_( make_impl_( fn, stacksize, alloc) )
	{}

	tasklet( tasklet const& other);

	tasklet & operator=( BOOST_COPY_ASSIGN_REF( tasklet) other);

	tasklet( BOOST_RV_REF( tasklet) other);

	tasklet & operator=( BOOST_RV_REF( tasklet) other);

	typedef detail::tasklet_base::ptr::unspecified_bool_type	unspecified_bool_type;

	operator unspecified_bool_type() const;

	bool operator!() const;

	void swap( tasklet & other);

	id get_id() const;

	bool operator==( tasklet const& other) const;
	bool operator!=( tasklet const& other) const;

	bool is_alive() const;

	int priority() const;

	void priority( int);

	void interrupt();

	bool interruption_requested() const;

	void cancel();

	void join();
};

class BOOST_TASKLET_DECL tasklet::id
{
private:
	friend class tasklet;

	uint64_t		id_;

	explicit id( detail::tasklet_base::ptr info) :
		id_( reinterpret_cast< uint64_t >( info.get() ) )
	{}

public:
	id() :
		id_( 0)
	{}

	bool operator==( id const& other) const
	{ return id_ == other.id_; }

	bool operator!=( id const& other) const
	{ return id_ != other.id_; }
	
	bool operator<( id const& other) const
	{ return id_ < other.id_; }
	
	bool operator>( id const& other) const
	{ return other.id_ < id_; }
	
	bool operator<=( id const& other) const
	{ return !( other.id_ < id_); }
	
	bool operator>=( id const& other) const
	{ return ! ( id_ < other.id_); }

	template< typename charT, class traitsT >
	friend std::basic_ostream< charT, traitsT > &
	operator<<( std::basic_ostream< charT, traitsT > & os, id const& other)
	{
		if ( 0 != other.id_)
			return os << other.id_;
		else
			return os << "{not-a-tasklet}";
	}
};

template< typename Fn >
tasklet make_tasklet( Fn fn, std::size_t stacksize)
{ return tasklet( fn, stacksize); }

#define BOOST_TASKLET_MAKE_TASKLET_FUNCTION(z, n, unused) \
template< typename Fn, BOOST_PP_ENUM_PARAMS(n, typename A) > \
tasklet make_tasklet( Fn fn, BOOST_ENUM_TASKLET_ARGS(n), std::size_t stacksize) \
{ return tasklet( fn, BOOST_PP_ENUM_PARAMS(n, a), stacksize); }

BOOST_PP_REPEAT_FROM_TO( 1, BOOST_TASKLET_MAX_ARITY, BOOST_TASKLET_MAKE_TASKLET_FUNCTION, ~)

#undef BOOST_TASKLET_MAKE_TASKLET_FUNCTION
#undef BOOST_ENUM_TASKLET_ARGS
#undef BOOST_TASKLET_ARG
#undef BOOST_TASKLET_MAX_ARITY

}

using tasklets::tasklet;

inline
void swap( tasklet & l, tasklet & r)
{ return l.swap( r); }

}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_TASKLET_H
