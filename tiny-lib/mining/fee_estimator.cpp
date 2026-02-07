#include "mining/fee_estimator.hpp"

#include <algorithm>
#include <ranges>

#include "core/mempool.hpp"
#include "core/unspent_tx_out.hpp"
#include "mining/pow.hpp"
#include "util/log.hpp"

std::recursive_mutex FeeEstimator::mutex;
std::deque<FeeEstimator::BlockFeeData> FeeEstimator::block_fee_history;

void FeeEstimator::record_block(const std::shared_ptr<Block>& block)
{
    std::scoped_lock lock(mutex);

    const auto block_id = block->id();

    for (const auto& existing : block_fee_history)
    {
        if (existing.block_id == block_id)
            return;
    }

    BlockFeeData data;
    data.block_id = block_id;

    for (const auto& tx : block->txs)
    {
        if (tx->is_coinbase())
            continue;

        const uint64_t fee = PoW::calculate_fees(tx);
        const uint32_t tx_size = tx->serialize().get_size();
        if (tx_size == 0)
            continue;

        const uint64_t fee_rate = fee / tx_size;
        data.fee_rates.push_back(fee_rate);
    }

    std::ranges::sort(data.fee_rates);

    block_fee_history.push_back(std::move(data));

    while (block_fee_history.size() > MAX_HISTORY_BLOCKS)
        block_fee_history.pop_front();

    LOG_TRACE("Fee estimator recorded block {} with {} transactions", block_id, data.fee_rates.size());
}

void FeeEstimator::unrecord_block(const std::string& block_id)
{
    std::scoped_lock lock(mutex);

    for (auto it = block_fee_history.begin(); it != block_fee_history.end(); ++it)
    {
        if (it->block_id == block_id)
        {
            block_fee_history.erase(it);

            LOG_TRACE("Fee estimator removed block {}", block_id);

            return;
        }
    }
}

uint64_t FeeEstimator::estimate_fee_rate(uint32_t target_blocks)
{
    std::scoped_lock lock(mutex);

    std::vector<uint64_t> all_rates;
    for (const auto& block_data : block_fee_history)
    {
        all_rates.insert(all_rates.end(), block_data.fee_rates.begin(), block_data.fee_rates.end());
    }

    const auto mempool_rates = collect_mempool_fee_rates();

    if (all_rates.empty() && mempool_rates.empty())
    {
        LOG_TRACE("No fee data available, returning default fee rate {}", DEFAULT_FEE_RATE);

        return DEFAULT_FEE_RATE;
    }

    std::ranges::sort(all_rates);

    uint64_t block_estimate = DEFAULT_FEE_RATE;
    if (!all_rates.empty())
    {
        double p;
        if (target_blocks <= 1)
            p = 0.85;
        else if (target_blocks <= 3)
            p = 0.60;
        else if (target_blocks <= 6)
            p = 0.35;
        else
            p = 0.10;

        block_estimate = percentile(all_rates, p);
    }

    uint64_t mempool_estimate = 0;
    if (!mempool_rates.empty())
    {
        double mempool_percentile;
        if (target_blocks <= 1)
            mempool_percentile = 0.90;
        else if (target_blocks <= 3)
            mempool_percentile = 0.50;
        else
            mempool_percentile = 0.25;

        auto sorted_mempool = mempool_rates;
        std::ranges::sort(sorted_mempool);
        mempool_estimate = percentile(sorted_mempool, mempool_percentile);
    }

    uint64_t estimate = std::max(block_estimate, mempool_estimate);
    estimate = std::max(estimate, MIN_RELAY_FEE_RATE);

    LOG_TRACE("Fee estimate for {} block target: {} coins/byte (block={}, mempool={})",
        target_blocks, estimate, block_estimate, mempool_estimate);

    return estimate;
}

uint64_t FeeEstimator::get_mempool_fee_rate_percentile(double p)
{
    std::scoped_lock lock(mutex);

    auto rates = collect_mempool_fee_rates();
    if (rates.empty())
        return DEFAULT_FEE_RATE;

    std::ranges::sort(rates);

    return percentile(rates, p);
}

void FeeEstimator::reset()
{
    std::scoped_lock lock(mutex);

    block_fee_history.clear();
}

uint64_t FeeEstimator::percentile(const std::vector<uint64_t>& sorted_rates, double p)
{
    if (sorted_rates.empty())
        return 0;

    if (sorted_rates.size() == 1)
        return sorted_rates[0];

    const double index = p * static_cast<double>(sorted_rates.size() - 1);
    const auto lower = static_cast<size_t>(index);
    const auto upper = lower + 1;

    if (upper >= sorted_rates.size())
        return sorted_rates.back();

    const double fraction = index - static_cast<double>(lower);

    return static_cast<uint64_t>(
        static_cast<double>(sorted_rates[lower]) * (1.0 - fraction) +
        static_cast<double>(sorted_rates[upper]) * fraction);
}

std::vector<uint64_t> FeeEstimator::collect_mempool_fee_rates()
{
    std::scoped_lock lock(Mempool::mutex);

    std::vector<uint64_t> rates;
    rates.reserve(Mempool::map.size());

    for (const auto& [tx_id, entry] : Mempool::map)
    {
        if (entry.serialized_size == 0)
            continue;

        rates.push_back(entry.fee_rate);
    }

    return rates;
}
