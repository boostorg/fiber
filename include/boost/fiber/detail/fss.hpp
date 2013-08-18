
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//          based on tss.hpp from boost.thread

#ifndef BOOST_FIBERS_DETAIL_FSS_H
#define BOOST_FIBERS_DETAIL_FSS_H

#include <cstddef>

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class BOOST_FIBERS_DECL fss_cleanup_function
{
private:
    std::size_t use_count_;

public:
    typedef intrusive_ptr< fss_cleanup_function >   ptr_t;

    fss_cleanup_function() :
        use_count_( 0)
    {}

    virtual ~fss_cleanup_function()
    {}

    virtual void operator()( void * data) = 0;

    friend inline void intrusive_ptr_add_ref(
        fss_cleanup_function * p) BOOST_NOEXCEPT
    { ++p->use_count_; }

    friend inline void intrusive_ptr_release(
        fss_cleanup_function * p)
    { if ( --p->use_count_ == 0) delete p; }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif //  BOOST_FIBERS_DETAIL_FSS_H
