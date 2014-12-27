
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FIBER_BASE_H
#define BOOST_FIBERS_DETAIL_FIBER_BASE_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <vector>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/execution_context.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_handle.hpp>
#include <boost/fiber/detail/flags.hpp>
#include <boost/fiber/detail/fss.hpp>
#include <boost/fiber/detail/rref.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/fiber_manager.hpp>
#include <boost/fiber/exceptions.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class BOOST_FIBERS_DECL fiber_base {
private:
    enum class fiber_status {
        ready = 0,
        running,
        waiting,
        terminated
    };

    struct BOOST_FIBERS_DECL fss_data {
        void                       *    vp;
        fss_cleanup_function::ptr_t     cleanup_function;

        fss_data() :
            vp( nullptr),
            cleanup_function() {
        }

        fss_data( void * vp_,
                  fss_cleanup_function::ptr_t const& fn) :
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
    std::chrono::high_resolution_clock::time_point  tp_;
    std::atomic< fiber_status >                     state_;
    std::atomic< int >                              flags_;
    std::atomic< int >                              priority_;
    spinlock                                        splk_;
    std::vector< fiber_handle >                     waiting_;
    std::exception_ptr                              except_;

    // worker fiber
    // generalized lambda captures are support by C++14
    template< typename StackAlloc, typename Fn >
    fiber_base( StackAlloc salloc, rref< Fn > rr) :
        use_count_( 0),
        ctx_( salloc,
              [=] () mutable {
                try {
                    BOOST_ASSERT( is_running() );
                    rr();
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
                fm_run();

                BOOST_ASSERT_MSG( false, "fiber already terminated");
              }),
        fss_data_(),
        tp_( (std::chrono::high_resolution_clock::time_point::max)() ),
        state_( fiber_status::ready),
        flags_( 0),
        priority_( 0),
        waiting_(),
        except_(),
        nxt( nullptr) {
    }

public:
    class id {
    private:
        fiber_base  *   impl_;

    public:
        id() noexcept :
            impl_( nullptr) {
        }

        explicit id( fiber_base * impl) noexcept :
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

    fiber_handle    nxt;

    // main-context fiber
    fiber_base() :
        use_count_( 1),
        ctx_( context::execution_context::current() ),
        fss_data_(),
        tp_( (std::chrono::high_resolution_clock::time_point::max)() ),
        state_( fiber_status::ready),
        flags_( 0),
        priority_( 0),
        waiting_(),
        except_(),
        nxt() {
    }

    // worker fiber
    // generalized lambda captures are support by C++14
    template< typename StackAlloc, typename Fn >
    explicit fiber_base( StackAlloc salloc, Fn && fn) :
        fiber_base( salloc, make_rref( std::move( fn) ) ) {
    }

    virtual ~fiber_base() {
        BOOST_ASSERT( waiting_.empty() );
    }

    id get_id() const noexcept {
        return id( const_cast< fiber_base * >( this) );
    }

    int priority() const noexcept {
        return priority_;
    }

    void priority( int prio) noexcept {
        priority_ = prio;
    }

    bool join( fiber_handle);

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
        fss_cleanup_function::ptr_t const& cleanup_fn,
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

        ctx_.jump_to();
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

    friend inline
    void intrusive_ptr_add_ref( fiber_base * f) {
        BOOST_ASSERT( nullptr != f);

        ++f->use_count_;
    }

    friend inline
    void intrusive_ptr_release( fiber_base * f) {
        BOOST_ASSERT( nullptr != f);

        if ( 0 == --f->use_count_) {
            BOOST_ASSERT( f->is_terminated() );
            delete f;
        }
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIBER_BASE_H
