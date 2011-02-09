//  (C) Copyright 2008-9 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_DETAIL_FUTURE_TRAITSHPP
#define BOOST_TASKS_DETAIL_FUTURE_TRAITSHPP

#include <algorithm>
#include <list>
#include <stdexcept>
#include <vector>

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/mpl/if.hpp>
#include <boost/next_prior.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_fundamental.hpp>

namespace boost {
namespace tasks {
namespace detail {

template<typename T>
struct future_traits
{
    typedef boost::scoped_ptr<T> storage_type;
#ifdef BOOST_HAS_RVALUE_REFS
	typedef T const& source_reference_type;
	struct dummy;
	typedef typename boost::mpl::if_<boost::is_fundamental<T>,dummy&,T&&>::type rvalue_source_type;
	typedef typename boost::mpl::if_<boost::is_fundamental<T>,T,T&&>::type move_dest_type;
#else
	typedef T& source_reference_type;
	typedef typename boost::mpl::if_<boost::is_convertible<T&, BOOST_RV_REF( T) >, BOOST_RV_REF( T),T const&>::type rvalue_source_type;
	typedef typename boost::mpl::if_<boost::is_convertible<T&,BOOST_RV_REF( T) >,BOOST_RV_REF( T),T>::type move_dest_type;
#endif

	static void init(storage_type& storage,source_reference_type t)
	{ storage.reset(new T(t)); }
	
	static void init(storage_type& storage,rvalue_source_type t)
	{ storage.reset(new T(static_cast<rvalue_source_type>(t))); }
	
	static void cleanup(storage_type& storage)
	{ storage.reset(); }
};
        
template<typename T>
struct future_traits<T&>
{
    typedef T* storage_type;
    typedef T& source_reference_type;
    struct rvalue_source_type {};
    typedef T& move_dest_type;

    static void init(storage_type& storage,T& t)
    { storage=&t; }

    static void cleanup(storage_type& storage)
    { storage=0; }
};

template<>
struct future_traits<void>
{
    typedef bool storage_type;
    typedef void move_dest_type;

    static void init(storage_type& storage)
    { storage=true; }

    static void cleanup(storage_type& storage)
    { storage=false; }
};

}}}

#endif // BOOST_TASKS_DETAIL_FUTURE_TRAITS_H
