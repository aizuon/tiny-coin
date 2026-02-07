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
    benchmark_correctness();
#endif

    std::cout << "\n=== Benchmark Complete ===" << std::endl;

    Crypto::cleanup();
    return EXIT_SUCCESS;
}
