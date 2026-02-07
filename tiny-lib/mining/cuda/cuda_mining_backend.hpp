#pragma once
#ifdef TINY_COIN_CUDA

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "mining/i_mining_backend.hpp"

class CudaMiningBackend : public IMiningBackend
{
public:
    CudaMiningBackend();
    ~CudaMiningBackend() override;

    CudaMiningBackend(const CudaMiningBackend&) = delete;
    CudaMiningBackend& operator=(const CudaMiningBackend&) = delete;

    std::string name() const override;

    MineResult mine(const std::vector<uint8_t>& header_prefix, const std::vector<uint8_t>& target_bytes,
        std::atomic_bool& interrupt) override;

    bool is_available() const;

private:
    int device_id_ = -1;
    bool available_ = false;

    static constexpr uint32_t BATCH_SIZE = 1 << 22;
};

#endif // TINY_COIN_CUDA
