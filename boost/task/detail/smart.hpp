
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DETAIL_SMART_H
#define BOOST_DETAIL_SMART_H

#include <boost/task/callable.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {
namespace detail {

struct replace_oldest
{
	template<
		typename PrioIndex,
		typename Value
	>
	void operator()( PrioIndex & idx, Value const& va)
	{
		typedef typename PrioIndex::iterator iterator;
		iterator i( idx.find( va.attr) );
		if ( i == idx.end() )
			idx.insert( va);
		else
			idx.replace( i, va);
	}
};

struct take_oldest
{
	class swapper
	{
	private:
		callable	&	ca_;

	public:
		swapper( callable & ca) :
			ca_( ca)
		{}

		template< typename Value >
		void operator()( Value & va)
		{ ca_.swap( va.ca); }
	};

	template< typename PrioIndex >
	void operator()( PrioIndex & idx, callable & ca)
	{
		typedef typename PrioIndex::iterator	iterator;
		iterator i( idx.begin() );
		BOOST_ASSERT( i != idx.end() );
		idx.modify( i, swapper( ca) );
		idx.erase( i);
	}
};

}}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_DETAIL_SMART_H
