#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "core/block.hpp"
#include "core/chain.hpp"
#include "core/mempool.hpp"
#include "core/net_params.hpp"
#include "core/tx.hpp"
#include "core/tx_in.hpp"
#include "core/tx_out.hpp"
#include "core/tx_out_point.hpp"
#include "core/unspent_tx_out.hpp"
#include "util/exceptions.hpp"
#include <gtest/gtest.h>

class MempoolPolicyTest : public ::testing::Test
{
protected:
    void SetUp() override { Chain::reset(); }
    void TearDown() override { Chain::reset(); }

    static std::shared_ptr<Tx> make_valid_tx(const std::string& source_id, uint64_t input_value, uint64_t output_value)
    {
        UTXO::add_to_map(std::make_shared<TxOut>(input_value, "addr"), source_id, 0, false, 1);
        auto outpoint = std::make_shared<TxOutPoint>(source_id, 0);
        auto tin = std::make_shared<TxIn>(outpoint, std::vector<uint8_t>{}, std::vector<uint8_t>{}, -1);
        auto tout = std::make_shared<TxOut>(output_value, "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs");
        return std::make_shared<Tx>(std::vector{ tin }, std::vector{ tout }, 0);
    }

    static void direct_insert(const std::shared_ptr<Tx>& tx, uint64_t fee)
    {
        Mempool::MempoolEntry entry;
        entry.tx = tx;
        entry.serialized_size = tx->serialize().get_size();
        entry.fee = fee;
        entry.fee_rate = entry.serialized_size > 0 ? fee / entry.serialized_size : 0;
        entry.insertion_time = std::chrono::steady_clock::now();
        Mempool::total_size_bytes += entry.serialized_size;
        Mempool::map[tx->id()] = std::move(entry);
    }
};

TEST_F(MempoolPolicyTest, DustOutputRejected)
{
    UTXO::add_to_map(std::make_shared<TxOut>(10000, "addr"), "dust_source", 0, false, 1);
    auto outpoint = std::make_shared<TxOutPoint>("dust_source", 0);
    auto tin = std::make_shared<TxIn>(outpoint, std::vector<uint8_t>{}, std::vector<uint8_t>{}, -1);
    auto dust_out = std::make_shared<TxOut>(NetParams::DUST_THRESHOLD - 1, "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs");
    auto tx = std::make_shared<Tx>(std::vector{ tin }, std::vector{ dust_out }, 0);

    Mempool::add_tx_to_mempool(tx);
    EXPECT_FALSE(Mempool::map.contains(tx->id()));
}

TEST_F(MempoolPolicyTest, AtDustThresholdAccepted)
{
    const uint64_t input_val = NetParams::DUST_THRESHOLD + 1000;
    auto tx = make_valid_tx("at_dust_source", input_val, NetParams::DUST_THRESHOLD);

    direct_insert(tx, 1000);
    EXPECT_TRUE(Mempool::map.contains(tx->id()));
}

TEST_F(MempoolPolicyTest, DuplicateTxIgnored)
{
    auto tx = make_valid_tx("dup_source", 10000, 9000);
    direct_insert(tx, 1000);
    EXPECT_TRUE(Mempool::map.contains(tx->id()));

    const auto size_before = Mempool::map.size();
    Mempool::add_tx_to_mempool(tx);
    EXPECT_EQ(size_before, Mempool::map.size());
}

TEST_F(MempoolPolicyTest, FindUtxoInMempool)
{
    auto tx = make_valid_tx("mempool_utxo_src", 50000, 40000);
    direct_insert(tx, 10000);

    auto outpoint = std::make_shared<TxOutPoint>(tx->id(), 0);
    auto found = Mempool::find_utxo_in_mempool(outpoint);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(40000, found->tx_out->value);
    EXPECT_FALSE(found->is_coinbase);

    auto bad_outpoint = std::make_shared<TxOutPoint>(tx->id(), 99);
    EXPECT_EQ(nullptr, Mempool::find_utxo_in_mempool(bad_outpoint));

    auto missing = std::make_shared<TxOutPoint>("nonexistent_tx_id", 0);
    EXPECT_EQ(nullptr, Mempool::find_utxo_in_mempool(missing));
}
