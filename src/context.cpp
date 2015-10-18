
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/context.hpp"

#include <cstdlib>
#include <new>

#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/interruption.hpp"
#include "boost/fiber/scheduler.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

// zero-initialization
thread_local
context *
context::active_;

static intrusive_ptr< context > make_dispatcher_context( scheduler * sched) {
    BOOST_ASSERT( nullptr != sched);
    default_stack salloc; // use default satck-size
    boost::context::stack_context sctx = salloc.allocate();
#if defined(BOOST_NO_CXX14_CONSTEXPR) || defined(BOOST_NO_CXX11_STD_ALIGN)
    // reserve space for control structure
    std::size_t size = sctx.size - sizeof( context);
    void * sp = static_cast< char * >( sctx.sp) - sizeof( context);
#else
    constexpr std::size_t func_alignment = 64; // alignof( context);
    constexpr std::size_t func_size = sizeof( context);
    // reserve space on stack
    void * sp = static_cast< char * >( sctx.sp) - func_size - func_alignment;
    // align sp pointer
    std::size_t space = func_size + func_alignment;
    sp = std::align( func_alignment, func_size, sp, space);
    BOOST_ASSERT( nullptr != sp);
    // calculate remaining size
    std::size_t size = sctx.size - ( static_cast< char * >( sctx.sp) - static_cast< char * >( sp) );
#endif
    // placement new of context on top of fiber's stack
    return intrusive_ptr< context >( 
            new ( sp) context(
                dispatcher_context,
                boost::context::preallocated( sp, size, sctx),
                salloc,
                sched) );
}

// zero-initialization
thread_local static std::size_t counter;

// schwarz counter
context_initializer::context_initializer() {
    if ( 0 == counter++) {
# if defined(BOOST_NO_CXX14_CONSTEXPR) || defined(BOOST_NO_CXX11_STD_ALIGN)
        // allocate memory for main context and scheduler
        constexpr std::size_t size = sizeof( context) + sizeof( scheduler);
        void * vp = std::malloc( size);
        if ( nullptr == vp) {
            throw std::bad_alloc();
        }
        // main fiber context of this thread
        context * main_ctx = new ( vp) context( main_context);
        // scheduler of this thread
        scheduler * sched = new ( static_cast< char * >( vp) + sizeof( context) ) scheduler();
        // attach main context to scheduler
        sched->attach_main_context( main_ctx);
        // create and attach dispatcher context to scheduler
        sched->attach_dispatcher_context(
                make_dispatcher_context( sched) );
        // make main context to active context
        context::active_ = main_ctx;
# else
        constexpr std::size_t alignment = 64; // alignof( capture_t);
        constexpr std::size_t ctx_size = sizeof( context);
        constexpr std::size_t sched_size = sizeof( scheduler);
        constexpr std::size_t size = 2 * alignment + ctx_size + sched_size;
        void * vp = std::malloc( size);
        if ( nullptr == vp) {
            throw std::bad_alloc();
        }
        // reserve space for shift
        void * vp1 = static_cast< char * >( vp) + sizeof( int); 
        // align context pointer
        std::size_t space = ctx_size + alignment;
        vp1 = std::align( alignment, ctx_size, vp1, space);
        // reserves space for integer holding shifted size
        int * shift = reinterpret_cast< int * >( static_cast< char * >( vp1) - sizeof( int) );
        // store shifted size in fornt of context
        * shift = static_cast< char * >( vp1) - static_cast< char * >( vp);
        // main fiber context of this thread
        context * main_ctx = new ( vp1) context( main_context);
        vp1 = static_cast< char * >( vp1) + ctx_size;
        // align scheduler pointer
        space = sched_size + alignment;
        vp1 = std::align( alignment, sched_size, vp1, space);
        // scheduler of this thread
        scheduler * sched = new ( vp1) scheduler();
        // attach main context to scheduler
        sched->attach_main_context( main_ctx);
        // create and attach dispatcher context to scheduler
        sched->attach_dispatcher_context(
                make_dispatcher_context( sched) );
        // make main context to active context
        context::active_ = main_ctx;
# endif
    }
}

context_initializer::~context_initializer() {
    if ( 0 == --counter) {
        context * main_ctx = context::active_;
        BOOST_ASSERT( main_ctx->is_main_context() );
        scheduler * sched = main_ctx->get_scheduler();
        sched->~scheduler();
        main_ctx->~context();
# if defined(BOOST_NO_CXX14_CONSTEXPR) || defined(BOOST_NO_CXX11_STD_ALIGN)
        std::free( main_ctx);
# else
        int * shift = reinterpret_cast< int * >( reinterpret_cast< char * >( main_ctx) - sizeof( int) );
        void * vp = reinterpret_cast< char * >( main_ctx) - ( * shift);
        std::free( vp);
# endif
    }
}

context *
context::active() noexcept {
    // initialized the first time control passes; per thread
    thread_local static boost::context::detail::activation_record_initializer rec_initializer;
    // initialized the first time control passes; per thread
    thread_local static context_initializer ctx_initializer;
    return active_;
}

void
context::active( context * active) noexcept {
    BOOST_ASSERT( nullptr != active);
    active_ = active;
}

void
context::reset_active() noexcept {
    active_ = nullptr;
}

// main fiber context
context::context( main_context_t) :
    use_count_( 1), // allocated on main- or thread-stack
    flags_( flag_main_context),
    scheduler_( nullptr),
    ctx_( boost::context::execution_context::current() ),
    ready_hook_(),
    remote_ready_hook_(),
    sleep_hook_(),
    terminated_hook_(),
    wait_hook_(),
    worker_hook_(),
    tp_( (std::chrono::steady_clock::time_point::max)() ),
    fss_data_(),
    wait_queue_(),
    splk_(),
    properties_( nullptr) {
}

// dispatcher fiber context
context::context( dispatcher_context_t, boost::context::preallocated const& palloc,
                  default_stack const& salloc, scheduler * sched) :
    use_count_( 0), // scheduler will own dispatcher context
    flags_( flag_dispatcher_context),
    scheduler_( nullptr),
    ctx_( std::allocator_arg, palloc, salloc,
          [=] (void * vp) -> void {
            if ( nullptr != vp) {
                std::function< void() > * func( static_cast< std::function< void() > * >( vp) );
                ( * func)();
            }
            // execute scheduler::dispatch()
            sched->dispatch();
            // dispatcher context should never return from scheduler::dispatch()
            BOOST_ASSERT_MSG( false, "disatcher fiber already terminated");
          }),
    ready_hook_(),
    remote_ready_hook_(),
    sleep_hook_(),
    terminated_hook_(),
    wait_hook_(),
    worker_hook_(),
    tp_( (std::chrono::steady_clock::time_point::max)() ),
    fss_data_(),
    wait_queue_(),
    splk_(),
    properties_( nullptr) {
}

context::~context() {
    BOOST_ASSERT( wait_queue_.empty() );
    BOOST_ASSERT( ! ready_is_linked() );
    BOOST_ASSERT( ! remote_ready_is_linked() );
    BOOST_ASSERT( ! sleep_is_linked() );
    BOOST_ASSERT( ! wait_is_linked() );
    delete properties_;
}

scheduler *
context::get_scheduler() const noexcept {
    return scheduler_;
}

context::id
context::get_id() const noexcept {
    return id( const_cast< context * >( this) );
}

std::function< void() > *
context::resume( std::function< void() > * func) {
    return static_cast< std::function< void() > * >( ctx_( func) );
}

void
context::suspend( std::function< void() > * func) noexcept {
    scheduler_->re_schedule( this, func);
}

void
context::join() {
    // get active context
    context * active_ctx = context::active();
    // context::join() is a interruption point
    this_fiber::interruption_point();
    // protect for concurrent access
    std::unique_lock< detail::spinlock > lk( splk_);
    // wait for context which is not terminated
    if ( 0 == ( flags_ & flag_terminated) ) {
        // push active context to wait-queue, member
        // of the context which has to be joined by
        // the active context
        active_ctx->wait_link( wait_queue_);
        lk.unlock();
        // suspend active context
        scheduler_->re_schedule( active_ctx);
        // remove from wait-queue
        active_ctx->wait_unlink();
        // active context resumed
        BOOST_ASSERT( context::active() == active_ctx);
    }
    // context::join() is a interruption point
    this_fiber::interruption_point();
}

void
context::yield() noexcept {
    // get active context
    context * active_ctx = context::active();
    // yield active context
    scheduler_->yield( active_ctx);
}

void
context::terminate() noexcept {
    // protect for concurrent access
    std::unique_lock< detail::spinlock > lk( splk_);
    // mark as terminated
    flags_ |= flag_terminated;
    // notify all waiting fibers
    while ( ! wait_queue_.empty() ) {
        context * ctx = & wait_queue_.front();
        // remove fiber from wait-queue
        wait_queue_.pop_front();
        // notify scheduler
        scheduler_->set_ready( ctx);
    }
    lk.unlock();
    // release fiber-specific-data
    for ( fss_data_t::value_type & data : fss_data_) {
        data.second.do_cleanup();
    }
    fss_data_.clear();
    // switch to another context
    scheduler_->set_terminated( this);
}

bool
context::wait_until( std::chrono::steady_clock::time_point const& tp,
                     std::function< void() > * func) noexcept {
    BOOST_ASSERT( nullptr != scheduler_);
    BOOST_ASSERT( this == active_);
    return scheduler_->wait_until( this, tp, func);
}

void
context::set_ready( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( this != ctx);
    BOOST_ASSERT( nullptr != scheduler_);
    BOOST_ASSERT( nullptr != ctx->scheduler_);
    // FIXME: comparing scheduler address' must be synchronized?
    //        what if ctx is migrated between threads
    //        (other scheduler assigned)
    if ( scheduler_ == ctx->scheduler_) {
        // local
        scheduler_->set_ready( ctx);
    } else {
        // remote
        ctx->scheduler_->set_remote_ready( ctx);
    }
}

void
context::interruption_blocked( bool blck) noexcept {
    if ( blck) {
        flags_ |= flag_interruption_blocked;
    } else {
        flags_ &= ~flag_interruption_blocked;
    }
}

void
context::request_interruption( bool req) noexcept {
    BOOST_ASSERT( ! is_main_context() && ! is_dispatcher_context() );
    if ( req) {
        flags_ |= flag_interruption_requested;
    } else {
        flags_ &= ~flag_interruption_requested;
    }
}

void
context::request_unwinding() noexcept {
    BOOST_ASSERT( ! is_main_context() );
    BOOST_ASSERT( ! is_dispatcher_context() );
    flags_ |= flag_forced_unwind;
}

void *
context::get_fss_data( void const * vp) const {
    uintptr_t key( reinterpret_cast< uintptr_t >( vp) );
    fss_data_t::const_iterator i( fss_data_.find( key) );
    return fss_data_.end() != i ? i->second.vp : nullptr;
}

void
context::set_fss_data( void const * vp,
                       detail::fss_cleanup_function::ptr_t const& cleanup_fn,
                       void * data,
                       bool cleanup_existing) {
    BOOST_ASSERT( cleanup_fn);
    uintptr_t key( reinterpret_cast< uintptr_t >( vp) );
    fss_data_t::iterator i( fss_data_.find( key) );
    if ( fss_data_.end() != i) {
        if( cleanup_existing) {
            i->second.do_cleanup();
        }
        if ( nullptr != data) {
            fss_data_.insert(
                    i,
                    std::make_pair(
                        key,
                        fss_data( data, cleanup_fn) ) );
        } else {
            fss_data_.erase( i);
        }
    } else {
        fss_data_.insert(
            std::make_pair(
                key,
                fss_data( data, cleanup_fn) ) );
    }
}

void
context::set_properties( fiber_properties * props) {
    delete properties_;
    properties_ = props;
}

bool
context::worker_is_linked() const {
    return worker_hook_.is_linked();
}

bool
context::terminated_is_linked() const {
    return terminated_hook_.is_linked();
}

bool
context::ready_is_linked() const {
    return ready_hook_.is_linked();
}

bool
context::remote_ready_is_linked() const {
    return remote_ready_hook_.is_linked();
}

bool
context::sleep_is_linked() const {
    return sleep_hook_.is_linked();
}

bool
context::wait_is_linked() const {
    return wait_hook_.is_linked();
}

void
context::worker_unlink() noexcept {
    worker_hook_.unlink();
}

void
context::ready_unlink() noexcept {
    ready_hook_.unlink();
}

void
context::remote_ready_unlink() noexcept {
    remote_ready_hook_.unlink();
}

void
context::sleep_unlink() noexcept {
    sleep_hook_.unlink();
}

void
context::wait_unlink() noexcept {
    wait_hook_.unlink();
}

void
context::attach( context * ctx) {
    BOOST_ASSERT( nullptr != ctx);
    scheduler_->attach_worker_context( ctx);
}

void
context::detach( context * ctx) {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( scheduler_ == ctx->scheduler_);
    scheduler_->detach_worker_context( ctx);
    ctx->scheduler_ = nullptr;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
