
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_CONTEXT_H
#define BOOST_FIBERS_CONTEXT_H

#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <type_traits>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/detail/invoke.hpp>
#include <boost/context/execution_context.hpp>
#include <boost/context/stack_context.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/parent_from_member.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/intrusive/set.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fss.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/fixedsize_stack.hpp>
#include <boost/fiber/properties.hpp>
#include <boost/fiber/segmented_stack.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class context;
struct context_initializer;
class fiber;
class scheduler;

namespace detail {

struct wait_tag;
typedef intrusive::list_member_hook<
    intrusive::tag< wait_tag >,
    intrusive::link_mode<
        intrusive::auto_unlink
    >
>                                 wait_hook;
// declaration of the functor that converts between
// the context class and the wait-hook
struct wait_functor {
    // required types
    typedef wait_hook               hook_type;
    typedef hook_type           *   hook_ptr;
    typedef const hook_type     *   const_hook_ptr;
    typedef context                 value_type;
    typedef value_type          *   pointer;
    typedef const value_type    *   const_pointer;

    // required static functions
    static hook_ptr to_hook_ptr( value_type &value);
    static const_hook_ptr to_hook_ptr( value_type const& value);
    static pointer to_value_ptr( hook_ptr n);
    static const_pointer to_value_ptr( const_hook_ptr n);
};

struct ready_tag;
typedef intrusive::list_member_hook<
    intrusive::tag< ready_tag >,
    intrusive::link_mode<
        intrusive::auto_unlink
    >
>                                       ready_hook;

struct remote_ready_tag;
typedef intrusive::list_member_hook<
    intrusive::tag< remote_ready_tag >,
    intrusive::link_mode<
        intrusive::auto_unlink
    >
>                                       remote_ready_hook;

struct sleep_tag;
typedef intrusive::set_member_hook<
    intrusive::tag< sleep_tag >,
    intrusive::link_mode<
        intrusive::auto_unlink
    >
>                                       sleep_hook;

struct terminated_tag;
typedef intrusive::list_member_hook<
    intrusive::tag< terminated_tag >,
    intrusive::link_mode<
        intrusive::auto_unlink
    >
>                                       terminated_hook;

struct worker_tag;
typedef intrusive::list_member_hook<
    intrusive::tag< worker_tag >,
    intrusive::link_mode<
        intrusive::auto_unlink
    >
>                                       worker_hook;

}

struct main_context_t {};
const main_context_t main_context{};

struct dispatcher_context_t {};
const dispatcher_context_t dispatcher_context{};

struct worker_context_t {};
const worker_context_t worker_context{};

class BOOST_FIBERS_DECL context {
private:
    friend struct context_initializer;
    friend class scheduler;

    enum flag_t {
        flag_main_context           = 1 << 1,
        flag_dispatcher_context     = 1 << 2,
        flag_worker_context         = 1 << 3,
        flag_terminated             = 1 << 4,
        flag_interruption_blocked   = 1 << 5,
        flag_interruption_requested = 1 << 6,
        flag_forced_unwind          = 1 << 7
    };

    struct BOOST_FIBERS_DECL fss_data {
        void                                *   vp;
        detail::fss_cleanup_function::ptr_t     cleanup_function;

        fss_data() :
            vp( nullptr),
            cleanup_function() {
        }

        fss_data( void * vp_,
                  detail::fss_cleanup_function::ptr_t const& fn) :
            vp( vp_),
            cleanup_function( fn) {
            BOOST_ASSERT( cleanup_function);
        }

        void do_cleanup() {
            ( * cleanup_function)( vp);
        }
    };

    typedef std::map< uintptr_t, fss_data >     fss_data_t;

    static thread_local context         *   active_;

#if ! defined(BOOST_FIBERS_NO_ATOMICS)
    std::atomic< std::size_t >              use_count_;
    std::atomic< int >                      flags_;
#else
    std::size_t                             use_count_;
    int                                     flags_;
#endif
    scheduler                           *   scheduler_;
    boost::context::execution_context       ctx_;

public:
    detail::ready_hook                      ready_hook_;
    detail::remote_ready_hook               remote_ready_hook_;
    detail::sleep_hook                      sleep_hook_;
    detail::terminated_hook                 terminated_hook_;
    detail::wait_hook                       wait_hook_;
    detail::worker_hook                     worker_hook_;
    std::chrono::steady_clock::time_point   tp_;

    typedef intrusive::list<
        context,
        intrusive::function_hook< detail::wait_functor >,
        intrusive::constant_time_size< false > >   wait_queue_t;

private:
    fss_data_t                              fss_data_;
    wait_queue_t                            wait_queue_;
    detail::spinlock                        splk_;
    fiber_properties                    *   properties_;

public:
    class id {
    private:
        context  *   impl_;

    public:
        id() noexcept :
            impl_( nullptr) {
        }

        explicit id( context * impl) noexcept :
            impl_( impl) {
        }

        bool operator==( id const& other) const noexcept {
            return impl_ == other.impl_;
        }

        bool operator!=( id const& other) const noexcept {
            return impl_ != other.impl_;
        }

        bool operator<( id const& other) const noexcept {
            return impl_ < other.impl_;
        }

        bool operator>( id const& other) const noexcept {
            return other.impl_ < impl_;
        }

        bool operator<=( id const& other) const noexcept {
            return ! ( * this > other);
        }

        bool operator>=( id const& other) const noexcept {
            return ! ( * this < other);
        }

        template< typename charT, class traitsT >
        friend std::basic_ostream< charT, traitsT > &
        operator<<( std::basic_ostream< charT, traitsT > & os, id const& other) {
            if ( nullptr != other.impl_) {
                return os << other.impl_;
            } else {
                return os << "{not-valid}";
            }
        }

        explicit operator bool() const noexcept {
            return nullptr != impl_;
        }

        bool operator!() const noexcept {
            return nullptr == impl_;
        }
    };

    static context * active() noexcept;

    static void active( context * active) noexcept;

    static void reset_active() noexcept;

    // main fiber context
    context( main_context_t);

    // dispatcher fiber context
    context( dispatcher_context_t, boost::context::preallocated const&,
             default_stack const&, scheduler *);

    // worker fiber context
    template< typename StackAlloc, typename Fn, typename ... Args >
    context( worker_context_t,
             boost::context::preallocated palloc, StackAlloc salloc,
             Fn && fn, Args && ... args) :
        use_count_( 1), // fiber instance or scheduler owner
        flags_( flag_worker_context),
        scheduler_( nullptr),
        ctx_( std::allocator_arg, palloc, salloc,
              // mutable: generated operator() is not const -> enables std::move( fn)
              // std::make_tuple: stores decayed copies of its args, implicitly unwraps std::reference_wrapper
              [=,fn_=std::forward< Fn >( fn),tpl_=std::make_tuple( std::forward< Args >( args) ...),
               ctx=boost::context::execution_context::current()] (void *) mutable -> void {
                try {
                    auto fn( std::move( fn_) );
                    auto tpl( std::move( tpl_) );
                    // jump back after initialization
                    void * vp = ctx();
                    // execute returned functor
                    if ( nullptr != vp) {
                        std::function< void() > * func( static_cast< std::function< void() > * >( vp) );
                        ( * func)();
                    }
                    // check for unwinding
                    if ( ! unwinding_requested() ) {
                        boost::context::detail::do_invoke( fn, tpl);
                    }
                } catch ( fiber_interrupted const&) {
                } catch ( forced_unwind const&) {
                } catch ( ... ) {
                    std::terminate();
                }
                // terminate context
                terminate();
                BOOST_ASSERT_MSG( false, "fiber already terminated");
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
        // switch for initialization
        ctx_();
    }

    virtual ~context();

    scheduler * get_scheduler() const noexcept;

    id get_id() const noexcept;

    std::function< void() > * resume( std::function< void() > *);

    void suspend( std::function< void() > * = nullptr) noexcept;

    void join();

    void yield() noexcept;

    void terminate() noexcept;

    bool wait_until( std::chrono::steady_clock::time_point const&,
                     std::function< void() > * = nullptr) noexcept;

    void set_ready( context *) noexcept;

    bool is_main_context() const noexcept {
        return 0 != ( flags_ & flag_main_context);
    }

    bool is_dispatcher_context() const noexcept {
        return 0 != ( flags_ & flag_dispatcher_context);
    }

    bool is_worker_context() const noexcept {
        return 0 != ( flags_ & flag_worker_context);
    }

    bool is_terminated() const noexcept {
        return 0 != ( flags_ & flag_terminated);
    }

    bool interruption_blocked() const noexcept {
        return 0 != ( flags_ & flag_interruption_blocked);
    }

    void interruption_blocked( bool blck) noexcept;

    bool interruption_requested() const noexcept {
        return 0 != ( flags_ & flag_interruption_requested);
    }

    void request_interruption( bool req) noexcept;

    bool unwinding_requested() const noexcept {
        return 0 != ( flags_ & flag_forced_unwind);
    }

    void request_unwinding() noexcept;

    void * get_fss_data( void const * vp) const;

    void set_fss_data(
        void const * vp,
        detail::fss_cleanup_function::ptr_t const& cleanup_fn,
        void * data,
        bool cleanup_existing);

    void set_properties( fiber_properties * props);

    fiber_properties * get_properties() const noexcept {
        return properties_;
    }

    bool ready_is_linked() const;

    bool remote_ready_is_linked() const;

    bool sleep_is_linked() const;

    bool terminated_is_linked() const;

    bool wait_is_linked() const;

    bool worker_is_linked() const;

    template< typename List >
    void ready_link( List & lst) noexcept {
        static_assert( std::is_same< typename List::value_traits::hook_type, detail::ready_hook >::value, "not a ready-queue");
        lst.push_back( * this);
    }

    template< typename List >
    void remote_ready_link( List & lst) noexcept {
        static_assert( std::is_same< typename List::value_traits::hook_type, detail::remote_ready_hook >::value, "not a remote ready-queue");
        lst.push_back( * this);
    }

    template< typename Set >
    void sleep_link( Set & set) noexcept {
        static_assert( std::is_same< typename Set::value_traits::hook_type,detail::sleep_hook >::value, "not a sleep-queue");
        set.insert( * this);
    }

    template< typename List >
    void terminated_link( List & lst) noexcept {
        static_assert( std::is_same< typename List::value_traits::hook_type, detail::terminated_hook >::value, "not a terminated-queue");
        lst.push_back( * this);
    }

    template< typename List >
    void wait_link( List & lst) noexcept {
        static_assert( std::is_same< typename List::value_traits::hook_type, detail::wait_hook >::value, "not a wait-queue");
        lst.push_back( * this);
    }

    template< typename List >
    void worker_link( List & lst) noexcept {
        static_assert( std::is_same< typename List::value_traits::hook_type, detail::worker_hook >::value, "not a worker-queue");
        lst.push_back( * this);
    }

    void ready_unlink() noexcept;

    void remote_ready_unlink() noexcept;

    void sleep_unlink() noexcept;

    void wait_unlink() noexcept;

    void worker_unlink() noexcept;

    void attach( context *);

    void migrate( context *);

    friend void intrusive_ptr_add_ref( context * ctx) {
        BOOST_ASSERT( nullptr != ctx);
        ++ctx->use_count_;
    }

    friend void intrusive_ptr_release( context * ctx) {
        BOOST_ASSERT( nullptr != ctx);
        if ( 0 == --ctx->use_count_) {
            ctx->~context();
        }
    }
};

struct context_initializer {
    context_initializer();
    ~context_initializer();
};

template< typename StackAlloc, typename Fn, typename ... Args >
static intrusive_ptr< context > make_worker_context( StackAlloc salloc, Fn && fn, Args && ... args) {
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
                worker_context,
                boost::context::preallocated( sp, size, sctx),
                salloc,
                std::forward< Fn >( fn),
                std::forward< Args >( args) ... ) );
}

namespace detail {

inline
wait_functor::hook_ptr wait_functor::to_hook_ptr( wait_functor::value_type & value) {
    return & value.wait_hook_;
}

inline
wait_functor::const_hook_ptr wait_functor::to_hook_ptr( wait_functor::value_type const& value) {
    return & value.wait_hook_;
}

inline
wait_functor::pointer wait_functor::to_value_ptr( wait_functor::hook_ptr n) {
    return intrusive::get_parent_from_member< context >( n, & context::wait_hook_);
}

inline
wait_functor::const_pointer wait_functor::to_value_ptr( wait_functor::const_hook_ptr n) {
    return intrusive::get_parent_from_member< context >( n, & context::wait_hook_);
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_CONTEXT_H
