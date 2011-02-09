
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_SYM_FIBER_H
#define BOOST_FIBERS_SYM_FIBER_H

#include <cstddef>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/move/move.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/sym_fiber_base.hpp>
#include <boost/fiber/detail/sym_fiber_object.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251)
# endif

namespace boost {
namespace fibers {

class BOOST_FIBER_DECL sym_fiber
{
private:
	struct dummy {};

	BOOST_COPYABLE_AND_MOVABLE( sym_fiber);

	detail::sym_fiber_base::ptr		impl_;

    sym_fiber( detail::sym_fiber_base::ptr const& impl);

	static detail::sym_fiber_base::ptr make_fiber_(
		void( * fn)(), std::size_t stacksize)
	{
		return detail::sym_fiber_base::ptr(
			new detail::sym_fiber_object< void(*)() >( fn, stacksize) );
	}

	static detail::sym_fiber_base::ptr make_fiber_( void( * fn)(),
            std::size_t stacksize, detail::sym_fiber_base::ptr & nxt)
	{
		return detail::sym_fiber_base::ptr(
			new detail::sym_fiber_object< void(*)() >( fn, stacksize, * nxt) );
	}

#ifdef BOOST_MSVC
	template< typename Fn >
	static detail::sym_fiber_base::ptr make_fiber_(
		Fn & fn, std::size_t stacksize)
	{
		return detail::sym_fiber_base::ptr(
			new detail::sym_fiber_object< Fn >( fn, stacksize) );
	}

	template< typename Fn >
	static detail::sym_fiber_base::ptr make_fiber_(
		Fn & fn, std::size_t stacksize, detail::sym_fiber_base::ptr & nxt)
	{
		return detail::sym_fiber_base::ptr(
			new detail::sym_fiber_object< Fn >( fn, stacksize, * nxt) );
	}
#else
	template< typename Fn >
	static detail::sym_fiber_base::ptr make_fiber_(
		Fn fn, std::size_t stacksize)
	{
		return detail::sym_fiber_base::ptr(
			new detail::sym_fiber_object< Fn >( fn, stacksize) );
	}

	template< typename Fn >
	static detail::sym_fiber_base::ptr make_fiber_(
		Fn fn, std::size_t stacksize, detail::sym_fiber_base::ptr & nxt)
	{
		return detail::sym_fiber_base::ptr(
			new detail::sym_fiber_object< Fn >( fn, stacksize, * nxt) );
	}
#endif

	template< typename Fn >
	static detail::sym_fiber_base::ptr make_fiber__(
		BOOST_RV_REF( Fn) fn, std::size_t stacksize)
	{
		return detail::sym_fiber_base::ptr(
			new detail::sym_fiber_object< Fn >( fn, stacksize) );
	}

	template< typename Fn >
	static detail::sym_fiber_base::ptr make_fiber__( BOOST_RV_REF( Fn) fn,
            std::size_t stacksize, detail::sym_fiber_base::ptr & nxt)
	{
		return detail::sym_fiber_base::ptr(
			new detail::sym_fiber_object< Fn >( fn, stacksize, * nxt) );
	}

public:
	static std::size_t default_stacksize;

    static sym_fiber from_current_context();

	class id;

	sym_fiber();

#ifdef BOOST_MSVC
	template< typename Fn >
	sym_fiber( Fn & fn, std::size_t stacksize) :
		impl_( make_fiber_( fn, stacksize) )
	{}

	template< typename Fn >
	sym_fiber( Fn & fn, std::size_t stacksize, sym_fiber & nxt) :
		impl_( make_fiber_( fn, stacksize, nxt.impl_) )
	{}
#else
	template< typename Fn >
	sym_fiber( Fn fn, std::size_t stacksize) :
		impl_( make_fiber_( fn, stacksize) )
	{}

	template< typename Fn >
	sym_fiber( Fn fn, std::size_t stacksize, sym_fiber & nxt) :
		impl_( make_fiber_( fn, stacksize, nxt.impl_) )
	{}
#endif

	template< typename Fn >
	sym_fiber( BOOST_RV_REF( Fn) fn, std::size_t stacksize) :
		impl_( make_fiber__( fn, stacksize) )
	{}

	template< typename Fn >
	sym_fiber( BOOST_RV_REF( Fn) fn, std::size_t stacksize, sym_fiber & nxt) :
		impl_( make_fiber__( fn, stacksize, nxt.impl_) )
	{}

#define BOOST_FIBERS_SYM_FIBER_ARG(z, n, unused) \
   BOOST_PP_CAT(A, n) BOOST_PP_CAT(a, n)
#define BOOST_ENUM_FIBERS_SYM_FIBER_ARGS(n) BOOST_PP_ENUM(n, BOOST_FIBERS_SYM_FIBER_ARG, ~)

#define BOOST_FIBERS_SYM_FIBER_CTOR(z, n, unused) \
	template< typename Fn, BOOST_PP_ENUM_PARAMS(n, typename A) > \
	sym_fiber( Fn fn, BOOST_ENUM_FIBERS_SYM_FIBER_ARGS(n), std::size_t stacksize) : \
		impl_( \
			make_fiber_( \
				boost::bind( boost::type< void >(), fn, BOOST_PP_ENUM_PARAMS(n, a) ), \
			   	stacksize) ) \
	{} \
\
	template< typename Fn, BOOST_PP_ENUM_PARAMS(n, typename A) > \
	sym_fiber( Fn fn, BOOST_ENUM_FIBERS_SYM_FIBER_ARGS(n), std::size_t stacksize, sym_fiber & nxt) : \
		impl_( \
			make_fiber_( \
				boost::bind( boost::type< void >(), fn, BOOST_PP_ENUM_PARAMS(n, a) ), \
			   	stacksize, nxt.impl_) ) \
	{} \

#ifndef BOOST_FIBERS_SYM_MAX_ARITY
#define BOOST_FIBERS_SYM_MAX_ARITY 10
#endif

BOOST_PP_REPEAT_FROM_TO( 1, BOOST_FIBERS_SYM_MAX_ARITY, BOOST_FIBERS_SYM_FIBER_CTOR, ~)

#undef BOOST_FIBERS_SYM_MAY_ARITY
#undef BOOST_FIBERS_SYM_FIBER_ARG
#undef BOOST_FIBERS_SYM_FIBER_ARGS
#undef BOOST_FIBERS_SYM_FIBER_CTOR

	sym_fiber( sym_fiber const& other);

	sym_fiber & operator=( BOOST_COPY_ASSIGN_REF( sym_fiber) other);

	sym_fiber( BOOST_RV_REF( sym_fiber) other);

	sym_fiber & operator=( BOOST_RV_REF( sym_fiber) other);

	typedef detail::sym_fiber_base::ptr::unspecified_bool_type	unspecified_bool_type;

	operator unspecified_bool_type() const;

	bool operator!() const;

	void swap( sym_fiber & other);

	id get_id() const;

	bool operator==( sym_fiber const& other) const;
	bool operator!=( sym_fiber const& other) const;

	void switch_to( sym_fiber & other);

	bool finished() const;
};

class BOOST_FIBER_DECL sym_fiber::id
{
private:
	friend class sym_fiber;

	boost::uint64_t		id_;

	explicit id( detail::sym_fiber_base::ptr const& info) :
		id_( reinterpret_cast< boost::uint64_t >( info.get() ) )
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
		if ( 0 != other.id_) return os << other.id_;
		else return os << "{not-a-fiber}";
	}
};

}

using fibers::sym_fiber;

inline
void swap( sym_fiber & l, sym_fiber & r)
{ return l.swap( r); }

}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_SYM_FIBER_H
