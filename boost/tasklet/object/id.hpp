
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKLETS_DETAIL_ID_H
#define BOOST_TASKLETS_DETAIL_ID_H

#include <iostream>

#include <boost/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace tasklets {
namespace object {

class id
{
private:
	void	const*	vp_;

public:
	template< typename T >
	id( T const& t) :
		vp_( static_cast< void const* >( & t) )
	{}

	bool operator==( id const& other) const
	{ return vp_ == other.vp_; }

	bool operator!=( id const& other) const
	{ return vp_ != other.vp_; }

	bool operator<( id const& other) const
	{ return vp_ < other.vp_; }

	bool operator>( id const& other) const
	{ return other.vp_ < vp_; }

	bool operator<=( id const& other) const
	{ return !( other.vp_ < vp_); }

	bool operator>=( id const& other) const
	{ return ! ( vp_ < other.vp_); }

	template< typename charT, class traitsT >
	friend std::basic_ostream< charT, traitsT > &
	operator<<( std::basic_ostream< charT, traitsT > & os, id const& other)
	{
		if ( other.vp_)
			return os << other.vp_;
		else
			return os << "{not-a-object}";
	}
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_DETAIL_ID_H
