#pragma once
#ifdef __APPLE__

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "mining/i_mining_backend.hpp"

struct MetalContext;

class MetalMiningBackend : public IMiningBackend
{
public:
    MetalMiningBackend();
    ~MetalMiningBackend() override;

    MetalMiningBackend(const MetalMiningBackend&) = delete;
    MetalMiningBackend& operator=(const MetalMiningBackend&) = delete;

    std::string name() const override;

    MineResult mine(const std::vector<uint8_t>& header_prefix, const std::vector<uint8_t>& target_bytes,
        std::atomic_bool& interrupt) override;

    bool is_available() const;

private:
    std::unique_ptr<MetalContext> ctx_;
    bool available_ = false;

    static constexpr uint32_t BATCH_SIZE = 1 << 22;
};

#endif // __APPLE__
