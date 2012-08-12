
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_ID_H
#define BOOST_FIBERS_DETAIL_ID_H

#include <iostream>

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class BOOST_FIBERS_DECL oid
{
private:
	friend class fibers::auto_reset_event;
	friend class fibers::condition;
    friend class fibers::count_down_event;
	friend class fibers::manual_reset_event;
	friend class fibers::mutex;
	friend class fibers::fibers;

	intptr_t		id_;

	template< typename T >
	oid( T * t) :
		id_( t ? reinterpret_cast< intptr_t >( t) : 0)
	{}

public:
	oid() :
		id_( 0)
	{}

	bool operator==( oid const& other) const
	{ return id_ == other.id_; }

	bool operator!=( oid const& other) const
	{ return id_ != other.id_; }
	
	bool operator<( oid const& other) const
	{ return id_ < other.id_; }
	
	bool operator>( oid const& other) const
	{ return other.id_ < id_; }
	
	bool operator<=( oid const& other) const
	{ return !( other.id_ < id_); }
	
	bool operator>=( oid const& other) const
	{ return ! ( id_ < other.id_); }

	template< typename charT, class traitsT >
	friend std::basic_ostream< charT, traitsT > &
	operator<<( std::basic_ostream< charT, traitsT > & os, oid const& other)
	{
		if ( 0 != other.id_)
			return os << "0x" << std::hex << other.id_;
		else
			return os << "{not-valid}";
	}

    operator bool() const
    { return 0 != id_; }

    bool operator!() const
    { return 0 == id_; }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_ID_H
