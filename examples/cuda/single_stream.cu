#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <tuple>

#include <cuda.h>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/fiber/all.hpp>
#include <boost/fiber/cuda/waitfor.hpp>

__global__
void kernel( int size, int * a, int * b, int * c) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    if ( idx < size) {
        int idx1 = (idx + 1) % 256;
        int idx2 = (idx + 2) % 256;
        float as = (a[idx] + a[idx1] + a[idx2]) / 3.0f;
        float bs = (b[idx] + b[idx1] + b[idx2]) / 3.0f;
        c[idx] = (as + bs) / 2;
    }
}

int main() {
    try {
        bool done = false;
        boost::fibers::fiber f1([&done]{
            std::cout << "f1: entered" << std::endl;
            try {
                cudaStream_t stream;
                cudaStreamCreate( & stream);
                int size = 1024 * 1024;
                int full_size = 20 * size;
                int * host_a, * host_b, * host_c;
                cudaHostAlloc( & host_a, full_size * sizeof( int), cudaHostAllocDefault);
                cudaHostAlloc( & host_b, full_size * sizeof( int), cudaHostAllocDefault);
                cudaHostAlloc( & host_c, full_size * sizeof( int), cudaHostAllocDefault);
                int * dev_a, * dev_b, * dev_c;
                cudaMalloc( & dev_a, size * sizeof( int) );
                cudaMalloc( & dev_b, size * sizeof( int) );
                cudaMalloc( & dev_c, size * sizeof( int) );
                std::minstd_rand generator;
                std::uniform_int_distribution<> distribution(1, 6);
                for ( int i = 0; i < full_size; ++i) {
                    host_a[i] = distribution( generator);
                    host_b[i] = distribution( generator);
                }
                for ( int i = 0; i < full_size; i += size) {
                    cudaMemcpyAsync( dev_a, host_a + i, size * sizeof( int), cudaMemcpyHostToDevice, stream);
                    cudaMemcpyAsync( dev_b, host_b + i, size * sizeof( int), cudaMemcpyHostToDevice, stream);
                    kernel<<< size / 256, 256, 0, stream >>>( size, dev_a, dev_b, dev_c);
                    cudaMemcpyAsync( host_c + i, dev_c, size * sizeof( int), cudaMemcpyDeviceToHost, stream);
                }
                auto result = boost::fibers::cuda::waitfor_all( stream);
                BOOST_ASSERT( stream == std::get< 0 >( result) );
                BOOST_ASSERT( cudaSuccess == std::get< 1 >( result) );
                cudaFreeHost( host_a);
                cudaFreeHost( host_b);
                cudaFreeHost( host_c);
                cudaFree( dev_a);
                cudaFree( dev_b);
                cudaFree( dev_c);
                cudaStreamDestroy( stream);
                done = true;
            } catch ( std::exception const& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
            }
            std::cout << "f1: leaving" << std::endl;
        });
        boost::fibers::fiber f2([&done]{
            std::cout << "f2: entered" << std::endl;
            while ( ! done) {
                std::cout << "f2: sleeping" << std::endl;
                boost::this_fiber::sleep_for( std::chrono::milliseconds( 1 ) );
            }
            std::cout << "f2: leaving" << std::endl;
        });
        f1.join();
        f2.join();
        std::cout << "done." << std::endl;
        return EXIT_SUCCESS;
    } catch ( std::exception const& e) {
        std::cerr << "exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "unhandled exception" << std::endl;
    }
	return EXIT_FAILURE;
}
