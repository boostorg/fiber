
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ASYM_FIBER_H
#define BOOST_FIBERS_ASYM_FIBER_H

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
#include <boost/fiber/detail/asym_fiber_base.hpp>
#include <boost/fiber/detail/asym_fiber_object.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251)
# endif

namespace boost {
namespace fibers {

class BOOST_FIBER_DECL asym_fiber
{
private:
	struct dummy {};

	BOOST_COPYABLE_AND_MOVABLE( asym_fiber);

	detail::asym_fiber_base::ptr		impl_;

	static detail::asym_fiber_base::ptr make_fiber_(
		void( * fn)(), std::size_t stacksize, bool do_return)
	{
		return detail::asym_fiber_base::ptr(
			do_return
			? new detail::asym_fiber_object< void(*)() >( fn, stacksize, detail::asym_fiber_base::do_return)
			: new detail::asym_fiber_object< void(*)() >( fn, stacksize, detail::asym_fiber_base::do_not_return) );
	}

#ifdef BOOST_MSVC
	template< typename Fn >
	static detail::asym_fiber_base::ptr make_fiber_(
		Fn & fn, std::size_t stacksize, bool do_return)
	{
		return detail::asym_fiber_base::ptr(
			do_return
			? new detail::asym_fiber_object< Fn >( fn, stacksize, detail::asym_fiber_base::do_return)
			: new detail::asym_fiber_object< Fn >( fn, stacksize, detail::asym_fiber_base::do_not_return) );
	}
#else
	template< typename Fn >
	static detail::asym_fiber_base::ptr make_fiber_(
		Fn fn, std::size_t stacksize, bool do_return)
	{
		return detail::asym_fiber_base::ptr(
			do_return
			? new detail::asym_fiber_object< Fn >( fn, stacksize, detail::asym_fiber_base::do_return)
			: new detail::asym_fiber_object< Fn >( fn, stacksize, detail::asym_fiber_base::do_not_return) );
	}
#endif

	template< typename Fn >
	static detail::asym_fiber_base::ptr make_fiber__(
		BOOST_RV_REF( Fn) fn, std::size_t stacksize, bool do_return)
	{
		return detail::asym_fiber_base::ptr(
			do_return
			? new detail::asym_fiber_object< Fn >( fn, stacksize, detail::asym_fiber_base::do_return)
			: new detail::asym_fiber_object< Fn >( fn, stacksize, detail::asym_fiber_base::do_not_return) );
	}

public:
	static std::size_t default_stacksize;

	class id;

	asym_fiber();

#ifdef BOOST_MSVC
	template< typename Fn >
	asym_fiber( Fn & fn, std::size_t stacksize, bool do_return = true) :
		impl_( make_fiber_( fn, stacksize, do_return) )
	{}
#else
	template< typename Fn >
	asym_fiber( Fn fn, std::size_t stacksize, bool do_return = true) :
		impl_( make_fiber_( fn, stacksize, do_return) )
	{}
#endif

	template< typename Fn >
	asym_fiber( BOOST_RV_REF( Fn) fn, std::size_t stacksize, bool do_return = true) :
		impl_( make_fiber__( fn, stacksize, do_return) )
	{}

#define BOOST_FIBERS_ASYM_FIBER_ARG(z, n, unused) \
   BOOST_PP_CAT(A, n) BOOST_PP_CAT(a, n)
#define BOOST_ENUM_FIBERS_ASYM_FIBER_ARGS(n) BOOST_PP_ENUM(n, BOOST_FIBERS_ASYM_FIBER_ARG, ~)

#define BOOST_FIBERS_ASYM_FIBER_CTOR(z, n, unused) \
	template< typename Fn, BOOST_PP_ENUM_PARAMS(n, typename A) > \
	asym_fiber( Fn fn, BOOST_ENUM_FIBERS_ASYM_FIBER_ARGS(n), std::size_t stacksize, bool do_return = true) : \
		impl_( \
			make_fiber_( \
				boost::bind( boost::type< void >(), fn, BOOST_PP_ENUM_PARAMS(n, a) ), \
			   	stacksize, do_return) ) \
	{} \

#ifndef BOOST_FIBERS_ASYM_MAX_ARITY
#define BOOST_FIBERS_ASYM_MAX_ARITY 10
#endif

BOOST_PP_REPEAT_FROM_TO( 1, BOOST_FIBERS_ASYM_MAX_ARITY, BOOST_FIBERS_ASYM_FIBER_CTOR, ~)

#undef BOOST_FIBERS_ASYM_MAY_ARITY
#undef BOOST_FIBERS_ASYM_FIBER_ARG
#undef BOOST_FIBERS_ASYM_FIBER_ARGS
#undef BOOST_FIBERS_ASYM_FIBER_CTOR

	asym_fiber( asym_fiber const& other);

	asym_fiber & operator=( BOOST_COPY_ASSIGN_REF( asym_fiber) other);

	asym_fiber( BOOST_RV_REF( asym_fiber) other);

	asym_fiber & operator=( BOOST_RV_REF( asym_fiber) other);

	typedef detail::asym_fiber_base::ptr::unspecified_bool_type	unspecified_bool_type;

	operator unspecified_bool_type() const;

	bool operator!() const;

	void swap( asym_fiber & other);

	id get_id() const;

	bool operator==( asym_fiber const& other) const;
	bool operator!=( asym_fiber const& other) const;

	void run();

	void yield();

	bool finished() const;
};

class BOOST_FIBER_DECL asym_fiber::id
{
private:
	friend class asym_fiber;

	boost::uint64_t		id_;

	explicit id( detail::asym_fiber_base::ptr const& info) :
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

using fibers::asym_fiber;

inline
void swap( asym_fiber & l, asym_fiber & r)
{ return l.swap( r); }

}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASYM_FIBER_H
