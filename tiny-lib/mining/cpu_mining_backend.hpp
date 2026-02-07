#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "mining/i_mining_backend.hpp"

class CpuMiningBackend : public IMiningBackend
{
public:
    std::string name() const override;

    MineResult mine(const std::vector<uint8_t>& header_prefix, const std::vector<uint8_t>& target_bytes,
        std::atomic_bool& interrupt) override;

private:
    static void mine_chunk(const std::vector<uint8_t>& prefix_bytes, const std::vector<uint8_t>& target_bytes,
        uint64_t start, uint64_t chunk_size, std::atomic_bool& found, std::atomic<uint64_t>& found_nonce,
        std::atomic<uint64_t>& hash_count, std::atomic_bool& interrupt);
};
