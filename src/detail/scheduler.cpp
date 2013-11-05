//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/detail/scheduler.hpp"

#include <boost/assert.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

static void cleanup_function( algorithm *) {}

thread_specific_ptr< algorithm > scheduler::instance_( cleanup_function);

notify::ptr_t
scheduler::make_notification( main_notifier & n) {
    notify::ptr_t p( & n);
    intrusive_ptr_add_ref( p.get() );
    return p;
}

algorithm *
scheduler::replace( algorithm * other) BOOST_NOEXCEPT
{
    algorithm * old = instance_.release();
    instance_.reset( other);
    return old;
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
