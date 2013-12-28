//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/detail/scheduler.hpp"

#include <boost/assert.hpp>

#include "boost/fiber/round_robin.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

static void deleter_fn( algorithm * algo) { delete algo; }
static void null_deleter_fn( algorithm *) {}

thread_specific_ptr< algorithm > scheduler::default_algo_( deleter_fn);
thread_specific_ptr< algorithm > scheduler::instance_( null_deleter_fn);

algorithm *
scheduler::instance()
{
    if ( ! instance_.get() )
    {
        default_algo_.reset( new round_robin() );
        instance_.reset( default_algo_.get() );
    }
    return instance_.get();
}

void
scheduler::replace( algorithm * other)
{
    BOOST_ASSERT( other);

    instance_.reset( other);
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
