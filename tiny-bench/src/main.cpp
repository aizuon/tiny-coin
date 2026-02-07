#include <cstdlib>
#include <iostream>
#include <string>

#include "crypto/crypto.hpp"
#include "util/log.hpp"

void benchmark_cpu();
void benchmark_correctness();

#ifdef __APPLE__
void benchmark_metal();
void benchmark_compare();
#endif

#ifdef TINY_COIN_CUDA
void benchmark_cuda();
void benchmark_cuda_compare();
#endif

int main(int argc, char** argv)
{
    Log::start_log(false);
    Crypto::init();

    std::cout << "=== tiny-coin Mining Benchmark ===" << std::endl;
    std::cout << std::endl;

    benchmark_cpu();

#ifdef __APPLE__
    benchmark_metal();
    benchmark_correctness();
    benchmark_compare();
#else
    std::cout << "\n[SKIP] Metal benchmarks not available on this platform" << std::endl;
#endif

#ifdef TINY_COIN_CUDA
    benchmark_cuda();
    benchmark_cuda_compare();
#else
    std::cout << "[SKIP] CUDA benchmarks not available on this platform" << std::endl;
#endif

    benchmark_correctness();

    std::cout << "\n=== Benchmark Complete ===" << std::endl;

    Crypto::cleanup();
    return EXIT_SUCCESS;
}
