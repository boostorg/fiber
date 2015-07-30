
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_CONTEXT_H
#define BOOST_FIBERS_FIBER_CONTEXT_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/all.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fss.hpp>
#include <boost/fiber/detail/invoke.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/fiber_manager.hpp>
#include <boost/fiber/exceptions.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class fiber_properties;

class BOOST_FIBERS_DECL fiber_context {
private:
    enum class fiber_status {
        ready = 0,
        running,
        waiting,
        terminated
    };

    enum flag_t {
        flag_main_fiber             = 1 << 1,
        flag_interruption_blocked   = 1 << 2,
        flag_interruption_requested = 1 << 3,
        flag_detached               = 1 << 4
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

    typedef std::map< uintptr_t, fss_data >   fss_data_t;

#if ! defined(BOOST_FIBERS_NO_ATOMICS)
    std::atomic< std::size_t >                      use_count_;
    std::atomic< fiber_status >                     state_;
    std::atomic< int >                              flags_;
#else
    std::size_t                                     use_count_;
    fiber_status                                    state_;
    int                                             flags_;
#endif
    detail::spinlock                                splk_;
    context::execution_context                      ctx_;
    fss_data_t                                      fss_data_;
    std::vector< fiber_context * >                  waiting_;
    std::exception_ptr                              except_;
    std::chrono::high_resolution_clock::time_point  tp_;
    fiber_properties                            *   properties_;

    // main fiber
    fiber_context() :
        use_count_( 1), // allocated on stack
        state_( fiber_status::running),
        flags_( flag_main_fiber),
        splk_(),
        ctx_( context::execution_context::current() ),
        fss_data_(),
        waiting_(),
        except_(),
        tp_( (std::chrono::high_resolution_clock::time_point::max)() ),
        properties_( nullptr),
        nxt( nullptr) {
    }

protected:
    virtual void deallocate() {
    }

public:
    class id {
    private:
        fiber_context  *   impl_;

    public:
        id() noexcept :
            impl_( nullptr) {
        }

        explicit id( fiber_context * impl) noexcept :
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

    fiber_context   *   nxt;

    static fiber_context * main_fiber();

    // worker fiber
    template< typename StackAlloc, typename Fn, typename ... Args >
    fiber_context( context::preallocated palloc,
                   StackAlloc salloc,
                   Fn && fn,
                   Args && ... args) :
        use_count_( 1), // allocated on stack
        state_( fiber_status::ready),
        flags_( 0),
        splk_(),
        ctx_( palloc, salloc,
              // general lambda with moveable
              [=,fn=std::forward< Fn >( fn),tpl=std::make_tuple( std::forward< Args >( args) ...)] () mutable -> decltype( auto) {
                try {
                    BOOST_ASSERT( is_running() );
                    detail::invoke_helper( std::move( fn), std::move( tpl) );
                    BOOST_ASSERT( is_running() );
                } catch( fiber_interrupted const&) {
                    except_ = std::current_exception();
                } catch( ... ) {
                    std::terminate();
                }

                // mark fiber as terminated
                set_terminated();

                // notify waiting (joining) fibers
                release();

                // switch to another fiber
                detail::scheduler::instance()->run();

                BOOST_ASSERT_MSG( false, "fiber already terminated");
              }),
        fss_data_(),
        waiting_(),
        except_(),
        tp_( (std::chrono::high_resolution_clock::time_point::max)() ),
        properties_( nullptr),
        nxt( nullptr) {
    }

    virtual ~fiber_context();

    id get_id() const noexcept {
        return id( const_cast< fiber_context * >( this) );
    }

    bool join( fiber_context *);

    bool interruption_blocked() const noexcept {
        return 0 != ( flags_ & flag_interruption_blocked);
    }

    void interruption_blocked( bool blck) noexcept;

    bool interruption_requested() const noexcept {
        return 0 != ( flags_ & flag_interruption_requested);
    }

    void request_interruption( bool req) noexcept;

    bool is_terminated() const noexcept {
        return fiber_status::terminated == state_;
    }

    bool is_ready() const noexcept {
        return fiber_status::ready == state_;
    }

    bool is_running() const noexcept {
        return fiber_status::running == state_;
    }

    bool is_waiting() const noexcept {
        return fiber_status::waiting == state_;
    }

    void set_terminated() noexcept {
        // TODO
#if ! defined(BOOST_FIBERS_NO_ATOMICS)
        fiber_status previous = state_.exchange( fiber_status::terminated);
#else
        fiber_status previous = state_;
        state_ = fiber_status::terminated;
#endif
        BOOST_ASSERT( fiber_status::running == previous);
        (void)previous;
    }

    void set_ready() noexcept {
        // TODO
#if ! defined(BOOST_FIBERS_NO_ATOMICS)
        fiber_status previous = state_.exchange( fiber_status::ready);
#else
        fiber_status previous = state_;
        state_ = fiber_status::ready;
#endif
        BOOST_ASSERT( fiber_status::waiting == previous || fiber_status::running == previous || fiber_status::ready == previous);
        (void)previous;
    }

    void set_running() noexcept {
        // TODO
#if ! defined(BOOST_FIBERS_NO_ATOMICS)
        fiber_status previous = state_.exchange( fiber_status::running);
#else
        fiber_status previous = state_;
        state_ = fiber_status::running;
#endif
        BOOST_ASSERT( fiber_status::ready == previous);
        (void)previous;
    }

    void set_waiting() noexcept {
        // TODO
#if ! defined(BOOST_FIBERS_NO_ATOMICS)
        fiber_status previous = state_.exchange( fiber_status::waiting);
#else
        fiber_status previous = state_;
        state_ = fiber_status::waiting;
#endif
        BOOST_ASSERT( fiber_status::running == previous);
        (void)previous;
    }

    void * get_fss_data( void const * vp) const;

    void set_fss_data(
        void const * vp,
        detail::fss_cleanup_function::ptr_t const& cleanup_fn,
        void * data,
        bool cleanup_existing);

    std::exception_ptr get_exception() const noexcept {
        return except_;
    }

    void set_exception( std::exception_ptr except) noexcept {
        except_ = except;
    }

    void resume() {
        BOOST_ASSERT( is_running() ); // set by the scheduler-algorithm

        ctx_();
    }

    std::chrono::high_resolution_clock::time_point const& time_point() const noexcept {
        return tp_;
    }

    void time_point( std::chrono::high_resolution_clock::time_point const& tp) {
        tp_ = tp;
    }

    void time_point_reset() {
        tp_ = (std::chrono::high_resolution_clock::time_point::max)();
    }

    void set_properties( fiber_properties* props);

    fiber_properties* get_properties() const
    {
        return properties_;
    }

    void release();

    friend void intrusive_ptr_add_ref( fiber_context * f) {
        BOOST_ASSERT( nullptr != f);
        ++f->use_count_;
    }

    friend void intrusive_ptr_release( fiber_context * f) {
        BOOST_ASSERT( nullptr != f);
        if ( 0 == --f->use_count_) {
            BOOST_ASSERT( f->is_terminated() );
            f->~fiber_context();
        }
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_FIBER_CONTEXT_H
