#include <memory>
#include <string>
#include <vector>

#include "core/block.hpp"
#include "core/chain.hpp"
#include "core/mempool.hpp"
#include "core/tx.hpp"
#include "core/tx_in.hpp"
#include "core/tx_out.hpp"
#include "core/tx_out_point.hpp"
#include "core/unspent_tx_out.hpp"
#include "mining/fee_estimator.hpp"
#include "mining/pow.hpp"
#include <gtest/gtest.h>

class FeeEstimatorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Chain::reset();
    }

    void TearDown() override
    {
        Chain::reset();
    }

    static std::shared_ptr<Tx> make_coinbase_tx(uint64_t value)
    {
        auto tx_in = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
        auto tx_out = std::make_shared<TxOut>(value, "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs");
        return std::make_shared<Tx>(std::vector{ tx_in }, std::vector{ tx_out }, 0);
    }

    static std::shared_ptr<Tx> make_fee_tx(const std::string& source_id, uint64_t input_value, uint64_t output_value)
    {
        UTXO::add_to_map(std::make_shared<TxOut>(input_value, "addr"), source_id, 0, false, 1);
        auto outpoint = std::make_shared<TxOutPoint>(source_id, 0);
        auto tin = std::make_shared<TxIn>(outpoint, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
        auto tout = std::make_shared<TxOut>(output_value, "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs");
        return std::make_shared<Tx>(std::vector{ tin }, std::vector{ tout }, 0);
    }

    static std::shared_ptr<Block> make_block_with_fee_tx(const std::string& source_id,
        uint64_t input_value, uint64_t output_value, int64_t timestamp = 1501821412, uint64_t nonce = 0)
    {
        auto coinbase = make_coinbase_tx(5000000000);
        auto tx = make_fee_tx(source_id, input_value, output_value);
        return std::make_shared<Block>(0, "", "merkle", timestamp, 24, nonce,
            std::vector<std::shared_ptr<Tx>>{coinbase, tx});
    }
};

TEST_F(FeeEstimatorTest, DefaultFeeRateWithNoData)
{
    EXPECT_EQ(FeeEstimator::estimate_fee_rate(1), FeeEstimator::DEFAULT_FEE_RATE);
    EXPECT_EQ(FeeEstimator::estimate_fee_rate(3), FeeEstimator::DEFAULT_FEE_RATE);
    EXPECT_EQ(FeeEstimator::estimate_fee_rate(6), FeeEstimator::DEFAULT_FEE_RATE);
    EXPECT_EQ(FeeEstimator::estimate_fee_rate(12), FeeEstimator::DEFAULT_FEE_RATE);
}

TEST_F(FeeEstimatorTest, RecordUnrecordAndReset)
{
    auto block = make_block_with_fee_tx("src_1", 1000000, 900000);
    FeeEstimator::record_block(block);

    const uint64_t rate = FeeEstimator::estimate_fee_rate(3);
    EXPECT_NE(rate, 0);
    EXPECT_GE(rate, FeeEstimator::MIN_RELAY_FEE_RATE);

    FeeEstimator::record_block(block);
    EXPECT_EQ(FeeEstimator::estimate_fee_rate(3), rate);

    auto coinbase_block = std::make_shared<Block>(0, "", "merkle2", 1501821500, 24, 99,
        std::vector<std::shared_ptr<Tx>>{make_coinbase_tx(5000000000)});
    FeeEstimator::record_block(coinbase_block);
    EXPECT_EQ(FeeEstimator::estimate_fee_rate(3), rate);

    FeeEstimator::unrecord_block(block->id());
    EXPECT_EQ(FeeEstimator::estimate_fee_rate(3), FeeEstimator::DEFAULT_FEE_RATE);

    FeeEstimator::record_block(block);
    EXPECT_NE(FeeEstimator::estimate_fee_rate(3), FeeEstimator::DEFAULT_FEE_RATE);
    FeeEstimator::reset();
    EXPECT_EQ(FeeEstimator::estimate_fee_rate(3), FeeEstimator::DEFAULT_FEE_RATE);
}

TEST_F(FeeEstimatorTest, HighPriorityHigherThanLow)
{
    for (int i = 0; i < 5; i++)
    {
        auto coinbase = make_coinbase_tx(5000000000);

        std::vector<std::shared_ptr<Tx>> txs{ coinbase };
        for (int j = 0; j < 10; j++)
        {
            const std::string source_id = "source_" + std::to_string(i * 10 + j);
            const uint64_t input_val = 100000 + static_cast<uint64_t>(j) * 50000;
            txs.push_back(make_fee_tx(source_id, input_val, 50000));
        }

        auto block = std::make_shared<Block>(0, "", "merkle", 1501821412 + i, 24, static_cast<uint64_t>(i), txs);
        FeeEstimator::record_block(block);
    }

    const uint64_t high = FeeEstimator::estimate_fee_rate(1);
    const uint64_t medium = FeeEstimator::estimate_fee_rate(3);
    const uint64_t low = FeeEstimator::estimate_fee_rate(6);
    const uint64_t economy = FeeEstimator::estimate_fee_rate(12);

    EXPECT_GE(high, medium);
    EXPECT_GE(medium, low);
    EXPECT_GE(low, economy);
}

TEST_F(FeeEstimatorTest, MempoolFeeRatePercentile)
{
    const std::string src1 = "mp_src_1";
    const std::string src2 = "mp_src_2";

    auto tx1 = make_fee_tx(src1, 500000, 100000);
    auto tx2 = make_fee_tx(src2, 1000000, 100000);

    {
        std::scoped_lock lock(Mempool::mutex);
        Mempool::MempoolEntry e1;
        e1.tx = tx1;
        e1.serialized_size = tx1->serialize().get_size();
        e1.fee = 500000 - 100000;
        e1.fee_rate = e1.serialized_size > 0 ? e1.fee / e1.serialized_size : 0;
        e1.insertion_time = std::chrono::steady_clock::now();
        Mempool::map[tx1->id()] = std::move(e1);

        Mempool::MempoolEntry e2;
        e2.tx = tx2;
        e2.serialized_size = tx2->serialize().get_size();
        e2.fee = 1000000 - 100000;
        e2.fee_rate = e2.serialized_size > 0 ? e2.fee / e2.serialized_size : 0;
        e2.insertion_time = std::chrono::steady_clock::now();
        Mempool::map[tx2->id()] = std::move(e2);
    }

    const uint64_t p50 = FeeEstimator::get_mempool_fee_rate_percentile(0.5);
    EXPECT_GT(p50, 0);

    const uint64_t p90 = FeeEstimator::get_mempool_fee_rate_percentile(0.9);
    EXPECT_GE(p90, p50);
}
