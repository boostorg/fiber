
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
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/fiber_manager.hpp>
#include <boost/fiber/exceptions.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

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
        flag_thread_affinity        = 1 << 4,
        flag_detached               = 1 << 5
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

    std::atomic< std::size_t >                      use_count_;
    context::execution_context                      ctx_;
    fss_data_t                                      fss_data_;
    std::atomic< fiber_status >                     state_;
    std::atomic< int >                              flags_;
    detail::spinlock                                splk_;
    std::vector< fiber_context * >                  waiting_;
    std::exception_ptr                              except_;
    std::chrono::high_resolution_clock::time_point  tp_;

    // main fiber
    fiber_context() :
        use_count_( 1), // allocated on stack
        ctx_( context::execution_context::current() ),
        fss_data_(),
        state_( fiber_status::running),
        flags_( flag_main_fiber | flag_thread_affinity),
        waiting_(),
        except_(),
        tp_( (std::chrono::high_resolution_clock::time_point::max)() ),
        nxt() {
    }

    // worker fiber
    template< typename StackAlloc, typename Fn, typename Tpl, std::size_t ... I >
    fiber_context( context::preallocated palloc, StackAlloc salloc,
                   Fn && fn_, Tpl && tpl_,
                   std::index_sequence< I ... >) :
        use_count_( 1), // allocated on stack
        ctx_( palloc, salloc,
              [=,fn=std::forward< Fn >( fn_),tpl=std::forward< Tpl >( tpl_)] () mutable {
                try {
                    BOOST_ASSERT( is_running() );
                    fn(
                        // std::tuple_element<> does not perfect forwarding
                        std::forward< decltype( std::get< I >( std::declval< Tpl >() ) ) >(
                            std::get< I >( std::forward< Tpl >( tpl) ) ) ... );
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
        state_( fiber_status::ready),
        flags_( 0),
        waiting_(),
        except_(),
        tp_( (std::chrono::high_resolution_clock::time_point::max)() ),
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
    explicit fiber_context( context::preallocated palloc, StackAlloc salloc,
                            Fn && fn, Args && ... args) :
        fiber_context( palloc, salloc,
                       std::forward< Fn >( fn),
                       std::make_tuple( std::forward< Args >( args) ... ),
                       std::index_sequence_for< Args ... >() ) {
    }

    virtual ~fiber_context() {
        BOOST_ASSERT( waiting_.empty() );
    }

    id get_id() const noexcept {
        return id( const_cast< fiber_context * >( this) );
    }

    bool join( fiber_context *);

    bool interruption_blocked() const noexcept {
        return 0 != ( flags_.load() & flag_interruption_blocked);
    }

    void interruption_blocked( bool blck) noexcept;

    bool interruption_requested() const noexcept {
        return 0 != ( flags_.load() & flag_interruption_requested);
    }

    void request_interruption( bool req) noexcept;

    bool thread_affinity() const noexcept {
        return 0 != ( flags_.load() & flag_thread_affinity);
    }

    void thread_affinity( bool req) noexcept;

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
        fiber_status previous = state_.exchange( fiber_status::terminated);
        BOOST_ASSERT( fiber_status::running == previous);
        (void)previous;
    }

    void set_ready() noexcept {
        fiber_status previous = state_.exchange( fiber_status::ready);
        BOOST_ASSERT( fiber_status::waiting == previous || fiber_status::running == previous || fiber_status::ready == previous);
        (void)previous;
    }

    void set_running() noexcept {
        fiber_status previous = state_.exchange( fiber_status::running);
        BOOST_ASSERT( fiber_status::ready == previous);
        (void)previous;
    }

    void set_waiting() noexcept {
        fiber_status previous = state_.exchange( fiber_status::waiting);
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