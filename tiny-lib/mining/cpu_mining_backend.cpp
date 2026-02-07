#include "mining/cpu_mining_backend.hpp"

#include <cstring>
#include <limits>

#include <boost/bind/bind.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/thread/thread.hpp>

#include "crypto/sha256.hpp"
#include "util/utils.hpp"

std::string CpuMiningBackend::name() const
{
    return "cpu";
}

MineResult CpuMiningBackend::mine(const std::vector<uint8_t>& header_prefix,
    const std::vector<uint8_t>& target_bytes, std::atomic_bool& interrupt)
{
    int32_t num_threads = static_cast<int32_t>(boost::thread::hardware_concurrency()) - 3;
    if (num_threads <= 0)
        num_threads = 1;

    const uint64_t chunk_size = std::numeric_limits<uint64_t>::max() / num_threads;
    std::atomic_bool found = false;
    std::atomic<uint64_t> found_nonce = 0;
    std::atomic<uint64_t> hash_count = 0;

    boost::thread_group thread_pool;
    for (int32_t i = 0; i < num_threads; i++)
    {
        thread_pool.create_thread(boost::bind(&CpuMiningBackend::mine_chunk,
            boost::cref(header_prefix), boost::cref(target_bytes),
            std::numeric_limits<uint64_t>::min() + chunk_size * i, chunk_size,
            boost::ref(found), boost::ref(found_nonce), boost::ref(hash_count),
            boost::ref(interrupt)));
    }
    thread_pool.join_all();

    MineResult result;
    result.found = found;
    result.nonce = found_nonce;
    result.hash_count = hash_count;
    return result;
}

void CpuMiningBackend::mine_chunk(const std::vector<uint8_t>& prefix_bytes_in,
    const std::vector<uint8_t>& target_bytes, uint64_t start, uint64_t chunk_size,
    std::atomic_bool& found, std::atomic<uint64_t>& found_nonce,
    std::atomic<uint64_t>& hash_count, std::atomic_bool& interrupt)
{
    auto prefix_bytes = prefix_bytes_in;
    const uint32_t nonce_offset = static_cast<uint32_t>(prefix_bytes.size());
    prefix_bytes.resize(nonce_offset + sizeof(uint64_t));

    uint64_t i = 0;
    uint64_t local_hash_count = 0;
    while (true)
    {
        uint64_t current_nonce = start + i;
        if constexpr (Utils::is_little_endian)
            std::memcpy(prefix_bytes.data() + nonce_offset, &current_nonce, sizeof(uint64_t));
        else
        {
            boost::endian::endian_reverse_inplace(current_nonce);
            std::memcpy(prefix_bytes.data() + nonce_offset, &current_nonce, sizeof(uint64_t));
        }

        const auto hash = SHA256::double_hash_binary(prefix_bytes);

        bool valid = false;
        for (size_t b = 0; b < 32; b++)
        {
            if (hash[b] < target_bytes[b])
            {
                valid = true;
                break;
            }
            if (hash[b] > target_bytes[b])
                break;
        }

        if (valid)
        {
            found = true;
            found_nonce = start + i;
            hash_count += local_hash_count;
            return;
        }

        ++local_hash_count;
        i++;
        if (i % 4096 == 0)
        {
            hash_count += local_hash_count;
            local_hash_count = 0;
            if (found || interrupt)
                return;
        }
        if (i == chunk_size)
        {
            hash_count += local_hash_count;
            return;
        }
    }
}
