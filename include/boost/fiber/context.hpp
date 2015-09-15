
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_CONTEXT_H
#define BOOST_FIBERS_CONTEXT_H

#include <atomic>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/execution_context.hpp>
#include <boost/context/stack_context.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/parent_from_member.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class context;
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
// the context class and the hook
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
        intrusive::safe_link
    >
>                                       ready_hook;

}

class BOOST_FIBERS_DECL context {
public:
    detail::wait_hook       wait_hook_;

    typedef intrusive::list< context,
                 intrusive::function_hook< detail::wait_functor >,
                 intrusive::constant_time_size< false > >           wait_queue_t;

private:
    enum class status {
        ready = 0,
        running,
        waiting,
        terminated
    };

    enum flag_t {
        flag_main_context       = 1 << 1,
        flag_dispatcher_context = 1 << 2
    };

    static thread_local context         *   active_;

#if ! defined(BOOST_FIBERS_NO_ATOMICS)
    std::atomic< std::size_t >              use_count_;
    std::atomic< status >                   state_;
    std::atomic< int >                      flags_;
#else
    std::size_t                             use_count_;
    status                                  state_;
    int                                     flags_;
#endif
    scheduler                           *   scheduler_;
    boost::context::execution_context       ctx_;
    wait_queue_t                            wait_queue_;

protected:
    virtual void deallocate() {
    }

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

    detail::ready_hook      ready_hook_;

    static context * active() noexcept;

    static void active( context * active) noexcept;

    // main fiber
    context() :
        use_count_( 1), // allocated on main- or thread-stack
        state_( status::running),
        flags_( flag_main_context),
        scheduler_( nullptr),
        ctx_( boost::context::execution_context::current() ),
        wait_queue_(),
        ready_hook_() {
    }

    // worker fiber
    template< typename StackAlloc, typename Fn, typename ... Args >
    context( boost::context::preallocated palloc, StackAlloc salloc,
             Fn && fn, Args && ... args) :
        use_count_( 1), // allocated on fiber-stack
        state_( status::ready),
        flags_( 0),
        scheduler_( nullptr),
        ctx_( palloc, salloc,
              // lambda, executed in execution context
              // mutable: generated operator() is not const -> enables std::move( fn)
              // std::make_tuple: stores decayed copies of its args, implicitly unwraps std::reference_wrapper
              [=,fn=std::forward< Fn >( fn),tpl=std::make_tuple( std::forward< Args >( args) ...)] () mutable -> void {
                //FIXME: invoke function
 
                // mark fiber as terminated
                set_terminated();

                // notify waiting (joining) fibers
                release();

                // FIXME: switch to another fiber

                BOOST_ASSERT_MSG( false, "fiber already terminated");
              }),
        wait_queue_(),
        ready_hook_() {
    }

    virtual ~context();

    void set_scheduler( scheduler *);

    scheduler * get_scheduler() const noexcept;

    id get_id() const noexcept;

    bool is_main_context() const noexcept {
        return 0 != ( flags_ & flag_main_context);
    }

    bool is_terminated() const noexcept {
        return status::terminated == state_;
    }

    bool is_ready() const noexcept {
        return status::ready == state_;
    }

    bool is_running() const noexcept {
        return status::running == state_;
    }

    bool is_waiting() const noexcept {
        return status::waiting == state_;
    }

    void set_terminated() noexcept {
#if ! defined(BOOST_FIBERS_NO_ATOMICS)
        status previous = state_.exchange( status::terminated);
#else
        status previous = state_;
        state_ = status::terminated;
#endif
        BOOST_ASSERT( status::running == previous);
        (void)previous;
    }

    void set_ready() noexcept {
#if ! defined(BOOST_FIBERS_NO_ATOMICS)
        status previous = state_.exchange( status::ready);
#else
        status previous = state_;
        state_ = status::ready;
#endif
        BOOST_ASSERT( status::waiting == previous || status::running == previous || status::ready == previous);
        (void)previous;
    }

    void set_running() noexcept {
#if ! defined(BOOST_FIBERS_NO_ATOMICS)
        status previous = state_.exchange( status::running);
#else
        status previous = state_;
        state_ = status::running;
#endif
        BOOST_ASSERT( status::ready == previous);
        (void)previous;
    }

    void set_waiting() noexcept {
#if ! defined(BOOST_FIBERS_NO_ATOMICS)
        status previous = state_.exchange( status::waiting);
#else
        status previous = state_;
        state_ = status::waiting;
#endif
        BOOST_ASSERT( status::waiting == previous || status::running == previous);
        (void)previous;
    }

    void resume();

    void release() noexcept;

    bool wait_is_linked();

    void wait_unlink();

    bool ready_is_linked();

    friend void intrusive_ptr_add_ref( context * f) {
        BOOST_ASSERT( nullptr != f);
        ++f->use_count_;
    }

    friend void intrusive_ptr_release( context * f) {
        BOOST_ASSERT( nullptr != f);
        if ( 0 == --f->use_count_) {
            BOOST_ASSERT( f->is_terminated() );
            f->~context();
        }
    }
};

template< typename StackAlloc, typename Fn, typename ... Args >
static intrusive_ptr< context > make_context( StackAlloc salloc, Fn && fn, Args && ... args) {
    boost::context::stack_context sctx( salloc.allocate() );
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
