
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_CUDA_WAITFOR_H
#define BOOST_FIBERS_CUDA_WAITFOR_H

#include <initializer_list>
#include <mutex>
#include <iostream>
#include <set>
#include <tuple>
#include <vector>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <cuda_runtime_api.h>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/is_all_same.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/mutex.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace cuda {
namespace detail {

// cudaStreamAddCallback is pending deprecation as of CUDA 10.0; the
// replacement is cudaLaunchHostFunc. Unlike the stream callback, the host
// function callback receives neither the originating stream nor a status
// code, so the two code paths need slightly different plumbing.
#if CUDART_VERSION >= 10000
#   define BOOST_FIBER_CUDA_USE_LAUNCHHOSTFUNC 1
#endif

#ifdef BOOST_FIBER_CUDA_USE_LAUNCHHOSTFUNC
template< typename Rendezvous >
static void CUDART_CB trampoline( void * vp) {
    Rendezvous * data = static_cast< Rendezvous * >( vp);
    data->notify();
}
#else
template< typename Rendezvous >
static void CUDART_CB trampoline( cudaStream_t st, cudaError_t status, void * vp) {
    Rendezvous * data = static_cast< Rendezvous * >( vp);
    data->notify( st, status);
}
#endif

class single_stream_rendezvous {
public:
    single_stream_rendezvous( cudaStream_t st) :
        st_{ st } {
#ifdef BOOST_FIBER_CUDA_USE_LAUNCHHOSTFUNC
        cudaError_t status = ::cudaLaunchHostFunc( st_, trampoline< single_stream_rendezvous >, this);
#else
        unsigned int flags = 0;
        cudaError_t status = ::cudaStreamAddCallback( st_, trampoline< single_stream_rendezvous >, this, flags);
#endif
        if ( cudaSuccess != status) {
            status_ = status;
            done_ = true;
        }
    }

#ifdef BOOST_FIBER_CUDA_USE_LAUNCHHOSTFUNC
    void notify() noexcept {
        std::unique_lock< mutex > lk{ mtx_ };
        status_ = cudaSuccess;
        done_ = true;
        lk.unlock();
        cv_.notify_one();
    }
#else
    void notify( cudaStream_t, cudaError_t status) noexcept {
        std::unique_lock< mutex > lk{ mtx_ };
        status_ = status;
        done_ = true;
        lk.unlock();
        cv_.notify_one();
    }
#endif

    std::tuple< cudaStream_t, cudaError_t > wait() {
        std::unique_lock< mutex > lk{ mtx_ };
        cv_.wait( lk, [this]{ return done_; });
        return std::make_tuple( st_, status_);
    }

private:
    mutex               mtx_{};
    condition_variable  cv_{};
    cudaStream_t        st_{};
    cudaError_t         status_{ cudaErrorUnknown };
    bool                done_{ false };
};

class many_streams_rendezvous {
public:
    many_streams_rendezvous( std::initializer_list< cudaStream_t > l) :
            stx_{ l } {
        results_.reserve( stx_.size() );
#ifdef BOOST_FIBER_CUDA_USE_LAUNCHHOSTFUNC
        hostfunc_ctx_.reserve( stx_.size() );
#endif
        for ( cudaStream_t st : stx_) {
#ifdef BOOST_FIBER_CUDA_USE_LAUNCHHOSTFUNC
            hostfunc_ctx_.push_back( hostfunc_context{ this, st } );
            cudaError_t status = ::cudaLaunchHostFunc( st, trampoline< hostfunc_context >, & hostfunc_ctx_.back() );
#else
            unsigned int flags = 0;
            cudaError_t status = ::cudaStreamAddCallback( st, trampoline< many_streams_rendezvous >, this, flags);
#endif
            if ( cudaSuccess != status) {
                std::unique_lock< mutex > lk{ mtx_ };
                stx_.erase( st);
                results_.push_back( std::make_tuple( st, status) );
            }
        }
    }

    void notify( cudaStream_t st, cudaError_t status) noexcept {
        std::unique_lock< mutex > lk{ mtx_ };
        stx_.erase( st);
        results_.push_back( std::make_tuple( st, status) );
        if ( stx_.empty() ) {
            lk.unlock();
            cv_.notify_one();
        }
    }

    std::vector< std::tuple< cudaStream_t, cudaError_t > > wait() {
        std::unique_lock< mutex > lk{ mtx_ };
        cv_.wait( lk, [this]{ return stx_.empty(); });
        return results_;
    }

private:
#ifdef BOOST_FIBER_CUDA_USE_LAUNCHHOSTFUNC
    struct hostfunc_context {
        many_streams_rendezvous *  rendezvous;
        cudaStream_t               st;

        void notify() noexcept {
            rendezvous->notify( st, cudaSuccess);
        }
    };
#endif
    mutex                                                   mtx_{};
    condition_variable                                      cv_{};
    std::set< cudaStream_t >                                stx_;
    std::vector< std::tuple< cudaStream_t, cudaError_t > >  results_;
#ifdef BOOST_FIBER_CUDA_USE_LAUNCHHOSTFUNC
    std::vector< hostfunc_context >                         hostfunc_ctx_;
#endif
};

}

void waitfor_all();

inline
std::tuple< cudaStream_t, cudaError_t > waitfor_all( cudaStream_t st) {
    detail::single_stream_rendezvous rendezvous( st);
    return rendezvous.wait();
}

template< typename ... STP >
std::vector< std::tuple< cudaStream_t, cudaError_t > > waitfor_all( cudaStream_t st0, STP ... stx) {
    static_assert( boost::fibers::detail::is_all_same< cudaStream_t, STP ...>::value, "all arguments must be of type `CUstream*`.");
    detail::many_streams_rendezvous rendezvous{ st0, stx ... };
    return rendezvous.wait();
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_CUDA_WAITFOR_H
