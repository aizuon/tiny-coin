#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

struct MineResult
{
    bool found = false;
    uint64_t nonce = 0;
    uint64_t hash_count = 0;
};

class IMiningBackend
{
public:
    virtual ~IMiningBackend() = default;

    virtual std::string name() const = 0;

    virtual MineResult mine(const std::vector<uint8_t>& header_prefix, const std::vector<uint8_t>& target_bytes,
        std::atomic_bool& interrupt) = 0;
};
