#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <vector>

#include "mining/cpu_mining_backend.hpp"
#include "mining/mining_backend_factory.hpp"
#include "mining/pow.hpp"
#include "crypto/sha256.hpp"
#include "util/utils.hpp"

#ifdef __APPLE__
#include "mining/metal/metal_mining_backend.hpp"
#endif

static std::vector<uint8_t> make_test_prefix()
{
    std::vector<uint8_t> prefix;
    uint64_t version = 0;
    prefix.resize(prefix.size() + 8);
    std::memcpy(prefix.data(), &version, 8);
    std::string prev_hash = "000000000000000000000000000000000000000000000000000000000000abcd";
    uint32_t len = static_cast<uint32_t>(prev_hash.size());
    prefix.resize(prefix.size() + 4);
    std::memcpy(prefix.data() + prefix.size() - 4, &len, 4);
    prefix.insert(prefix.end(), prev_hash.begin(), prev_hash.end());
    std::string merkle = "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789";
    len = static_cast<uint32_t>(merkle.size());
    prefix.resize(prefix.size() + 4);
    std::memcpy(prefix.data() + prefix.size() - 4, &len, 4);
    prefix.insert(prefix.end(), merkle.begin(), merkle.end());
    int64_t timestamp = 1700000000;
    prefix.resize(prefix.size() + 8);
    std::memcpy(prefix.data() + prefix.size() - 8, &timestamp, 8);
    uint8_t bits = 20;
    prefix.push_back(bits);
    return prefix;
}

static std::vector<uint8_t> make_target(uint8_t bits)
{
    uint256_t target = uint256_t(1) << (255 - bits);
    return PoW::target_to_bytes(target);
}

static bool verify_nonce(const std::vector<uint8_t>& prefix, uint64_t nonce, const std::vector<uint8_t>& target)
{
    auto full_header = prefix;
    full_header.resize(full_header.size() + 8);
    if constexpr (Utils::is_little_endian)
        std::memcpy(full_header.data() + prefix.size(), &nonce, 8);
    else
    {
        uint64_t le_nonce = nonce;
        auto* p = reinterpret_cast<uint8_t*>(&le_nonce);
        for (int i = 0; i < 4; i++)
            std::swap(p[i], p[7 - i]);
        std::memcpy(full_header.data() + prefix.size(), &le_nonce, 8);
    }

    auto hash = SHA256::double_hash_binary(full_header);
    for (size_t i = 0; i < 32; i++)
    {
        if (hash[i] < target[i])
            return true;
        if (hash[i] > target[i])
            return false;
    }
    return false;
}

struct BenchResult
{
    double seconds;
    uint64_t hash_count;
    uint64_t nonce;
    bool found;

    double hashes_per_sec() const { return seconds > 0 ? hash_count / seconds : 0; }
    double khs() const { return hashes_per_sec() / 1000.0; }
    double mhs() const { return hashes_per_sec() / 1000000.0; }
};

void benchmark_cpu()
{
    std::cout << "--- CPU Mining Benchmark ---" << std::endl;

    auto prefix = make_test_prefix();
    constexpr uint8_t bits = 20;
    auto target = make_target(bits);

    std::atomic_bool interrupt = false;
    CpuMiningBackend backend;

    auto start = std::chrono::high_resolution_clock::now();
    auto result = backend.mine(prefix, target, interrupt);
    auto end = std::chrono::high_resolution_clock::now();

    double seconds = std::chrono::duration<double>(end - start).count();

    std::cout << "  Backend:    " << backend.name() << std::endl;
    std::cout << "  Found:      " << (result.found ? "yes" : "no") << std::endl;
    std::cout << "  Nonce:      " << result.nonce << std::endl;
    std::cout << "  Hashes:     " << result.hash_count << std::endl;
    std::cout << "  Time:       " << std::fixed << std::setprecision(3) << seconds << " s" << std::endl;
    std::cout << "  Hash rate:  " << std::fixed << std::setprecision(2)
        << (seconds > 0 ? result.hash_count / seconds / 1000.0 : 0) << " kH/s" << std::endl;

    if (result.found)
    {
        bool valid = verify_nonce(prefix, result.nonce, target);
        std::cout << "  Valid:      " << (valid ? "yes" : "NO - INVALID!") << std::endl;
        assert(valid && "CPU mining produced invalid nonce!");
    }

    std::cout << std::endl;
}

#ifdef __APPLE__
void benchmark_metal()
{
    std::cout << "--- Metal GPU Mining Benchmark ---" << std::endl;

    auto prefix = make_test_prefix();
    constexpr uint8_t bits = 20;
    auto target = make_target(bits);

    MetalMiningBackend backend;
    if (!backend.is_available())
    {
        std::cout << "  [SKIP] Metal not available" << std::endl << std::endl;
        return;
    }

    std::atomic_bool interrupt = false;

    auto start = std::chrono::high_resolution_clock::now();
    auto result = backend.mine(prefix, target, interrupt);
    auto end = std::chrono::high_resolution_clock::now();

    double seconds = std::chrono::duration<double>(end - start).count();

    std::cout << "  Backend:    " << backend.name() << std::endl;
    std::cout << "  Found:      " << (result.found ? "yes" : "no") << std::endl;
    std::cout << "  Nonce:      " << result.nonce << std::endl;
    std::cout << "  Hashes:     " << result.hash_count << std::endl;
    std::cout << "  Time:       " << std::fixed << std::setprecision(3) << seconds << " s" << std::endl;
    std::cout << "  Hash rate:  " << std::fixed << std::setprecision(2)
        << (seconds > 0 ? result.hash_count / seconds / 1000.0 : 0) << " kH/s" << std::endl;

    if (result.found)
    {
        bool valid = verify_nonce(prefix, result.nonce, target);
        std::cout << "  Valid:      " << (valid ? "yes" : "NO - INVALID!") << std::endl;
        assert(valid && "Metal mining produced invalid nonce!");
    }

    std::cout << std::endl;
}

void benchmark_compare()
{
    std::cout << "--- CPU vs Metal Comparison ---" << std::endl;

    auto prefix = make_test_prefix();
    constexpr uint8_t bits = 20;
    auto target = make_target(bits);

    std::atomic_bool interrupt_cpu = false;
    CpuMiningBackend cpu_backend;

    auto cpu_start = std::chrono::high_resolution_clock::now();
    auto cpu_result = cpu_backend.mine(prefix, target, interrupt_cpu);
    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_seconds = std::chrono::duration<double>(cpu_end - cpu_start).count();
    double cpu_hps = cpu_seconds > 0 ? cpu_result.hash_count / cpu_seconds : 0;

    MetalMiningBackend metal_backend;
    if (!metal_backend.is_available())
    {
        std::cout << "  [SKIP] Metal not available for comparison" << std::endl << std::endl;
        return;
    }

    std::atomic_bool interrupt_metal = false;

    auto metal_start = std::chrono::high_resolution_clock::now();
    auto metal_result = metal_backend.mine(prefix, target, interrupt_metal);
    auto metal_end = std::chrono::high_resolution_clock::now();
    double metal_seconds = std::chrono::duration<double>(metal_end - metal_start).count();
    double metal_hps = metal_seconds > 0 ? metal_result.hash_count / metal_seconds : 0;

    std::cout << "  CPU:   " << std::fixed << std::setprecision(2) << cpu_hps / 1000.0 << " kH/s ("
        << std::setprecision(3) << cpu_seconds << " s)" << std::endl;
    std::cout << "  Metal: " << std::fixed << std::setprecision(2) << metal_hps / 1000.0 << " kH/s ("
        << std::setprecision(3) << metal_seconds << " s)" << std::endl;

    if (metal_hps > cpu_hps)
    {
        double speedup = metal_hps / cpu_hps;
        std::cout << "  Result: Metal is " << std::fixed << std::setprecision(2)
            << speedup << "x faster than CPU" << std::endl;
    }
    else
    {
        double factor = cpu_hps > 0 ? metal_hps / cpu_hps : 0;
        std::cout << "  Result: Metal is " << std::fixed << std::setprecision(2)
            << factor << "x of CPU speed (slower)" << std::endl;
    }

    assert(metal_hps > cpu_hps && "Metal GPU mining should be faster than CPU mining!");

    std::cout << "  PASS: Metal outperforms CPU" << std::endl;
    std::cout << std::endl;
}
#endif

void benchmark_correctness()
{
    std::cout << "--- Correctness Verification ---" << std::endl;

    auto prefix = make_test_prefix();
    constexpr uint8_t bits = 20;
    auto target = make_target(bits);

    std::atomic_bool interrupt_cpu = false;
    CpuMiningBackend cpu_backend;
    auto cpu_result = cpu_backend.mine(prefix, target, interrupt_cpu);

    std::cout << "  CPU found nonce:   " << cpu_result.nonce << " -> "
        << (verify_nonce(prefix, cpu_result.nonce, target) ? "VALID" : "INVALID") << std::endl;
    assert(cpu_result.found && "CPU should find a nonce");
    assert(verify_nonce(prefix, cpu_result.nonce, target) && "CPU nonce must be valid");

#ifdef __APPLE__
    MetalMiningBackend metal_backend;
    if (metal_backend.is_available())
    {
        std::atomic_bool interrupt_metal = false;
        auto metal_result = metal_backend.mine(prefix, target, interrupt_metal);

        std::cout << "  Metal found nonce: " << metal_result.nonce << " -> "
            << (verify_nonce(prefix, metal_result.nonce, target) ? "VALID" : "INVALID") << std::endl;
        assert(metal_result.found && "Metal should find a nonce");
        assert(verify_nonce(prefix, metal_result.nonce, target) && "Metal nonce must be valid");
    }
    else
    {
        std::cout << "  [SKIP] Metal not available" << std::endl;
    }
#endif

    std::cout << "  PASS: All backends produce valid nonces" << std::endl;
    std::cout << std::endl;
}
