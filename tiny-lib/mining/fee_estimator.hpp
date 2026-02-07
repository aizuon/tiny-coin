#pragma once
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/block.hpp"

class FeeEstimator
{
public:
    static constexpr uint32_t MAX_HISTORY_BLOCKS = 20;
    static constexpr uint64_t DEFAULT_FEE_RATE = 100;
    static constexpr uint64_t MIN_RELAY_FEE_RATE = 1;

    static std::recursive_mutex mutex;

    static void record_block(const std::shared_ptr<Block>& block);
    static void unrecord_block(const std::string& block_id);

    static uint64_t estimate_fee_rate(uint32_t target_blocks = 3);

    static uint64_t get_mempool_fee_rate_percentile(double p);

    static void reset();

private:
    struct BlockFeeData
    {
        std::string block_id;
        std::vector<uint64_t> fee_rates;
    };

    static std::deque<BlockFeeData> block_fee_history;

    static uint64_t percentile(const std::vector<uint64_t>& sorted_rates, double p);
    static std::vector<uint64_t> collect_mempool_fee_rates();
};
