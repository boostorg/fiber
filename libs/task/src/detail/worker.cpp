
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/task/detail/worker.hpp"

namespace boost {
namespace tasks {
namespace detail {

thread_specific_ptr< worker > worker::tss_;

const thread::id
worker::get_id() const
{ return impl_->get_id(); }

void
worker::join() const
{ impl_->join(); }

void
worker::interrupt() const
{ impl_->interrupt(); }

void
worker::put( callable const& ca)
{ impl_->put( ca); }

bool
worker::try_steal( callable & ca)
{ return impl_->try_steal( ca); }

void
worker::run()
{
	// FIXME: ugly
	worker::tss_.reset( new worker( * this) );
	impl_->run();
}

void
worker::yield()
{ impl_->yield(); }

worker *
worker::tss_get()
{ return worker::tss_.get(); }

}}}
