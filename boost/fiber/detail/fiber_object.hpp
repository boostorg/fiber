
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_STRATUM_OBJECT_H
#define BOOST_FIBERS_DETAIL_STRATUM_OBJECT_H

#include <cstddef>

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/ref.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename Fn >
class fiber_object : public fiber_base
{
private:
	Fn		    fn_;

	fiber_object( fiber_object &);
	fiber_object & operator=( fiber_object const&);

public:
	fiber_object( Fn fn, std::size_t size, bool preserve_fpu) :
		fiber_base( size, preserve_fpu),
		fn_( fn)
	{}

	fiber_object( BOOST_RV_REF( Fn) fn, std::size_t size, bool preserve_fpu) :
		fiber_base( size, preserve_fpu),
		fn_( boost::move( fn) )
    {}

	void exec()
	{ fn_(); }
};

template< typename Fn >
class fiber_object< reference_wrapper< Fn > > : public fiber_base
{
private:
	Fn		    fn_;

	fiber_object( fiber_object &);
	fiber_object & operator=( fiber_object const&);

public:
	fiber_object( reference_wrapper< Fn > fn, std::size_t size, bool preserve_fpu) :
		fiber_base( size, preserve_fpu),
		fn_( fn)
    {}

	void exec()
	{ fn_(); }
};

template< typename Fn >
class fiber_object< const reference_wrapper< Fn > > : public fiber_base
{
private:
	Fn		    fn_;

	fiber_object( fiber_object &);
	fiber_object & operator=( fiber_object const&);

public:
	fiber_object( const reference_wrapper< Fn > fn, std::size_t size, bool preserve_fpu) :
		fiber_base( size, preserve_fpu),
		fn_( fn)
    {}

	void exec()
	{ fn_(); }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_STRATUM_OBJECT_H
