#include <algorithm>
#include <chrono>
#include <memory>
#include <set>
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
#include <gtest/gtest.h>

class CPFPTest : public ::testing::Test
{
protected:
    void SetUp() override { Chain::reset(); }
    void TearDown() override { Chain::reset(); }

    static std::shared_ptr<Tx> make_tx(const std::string& source_id, int64_t source_idx,
        uint64_t input_value, uint64_t output_value,
        const std::string& to_addr = "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs")
    {
        UTXO::add_to_map(std::make_shared<TxOut>(input_value, "addr"), source_id, source_idx, false, 1);
        auto outpoint = std::make_shared<TxOutPoint>(source_id, source_idx);
        auto tin = std::make_shared<TxIn>(outpoint, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
        auto tout = std::make_shared<TxOut>(output_value, to_addr);
        return std::make_shared<Tx>(std::vector{ tin }, std::vector{ tout }, 0);
    }

    static void insert_into_mempool(const std::shared_ptr<Tx>& tx, uint64_t fee)
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

TEST_F(CPFPTest, HighFeeChildPullsLowFeeParent)
{
    auto parent_tx = make_tx("utxo_parent", 0, 1000000, 999800);
    const uint64_t parent_fee = 200;
    insert_into_mempool(parent_tx, parent_fee);

    auto child_outpoint = std::make_shared<TxOutPoint>(parent_tx->id(), 0);
    auto child_tin = std::make_shared<TxIn>(child_outpoint, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
    auto child_tout = std::make_shared<TxOut>(900000, "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs");
    auto child_tx = std::make_shared<Tx>(std::vector{ child_tin }, std::vector{ child_tout }, 0);
    const uint64_t child_fee = 99800;
    insert_into_mempool(child_tx, child_fee);

    auto medium_tx = make_tx("utxo_medium", 0, 500000, 490000);
    const uint64_t medium_fee = 10000;
    insert_into_mempool(medium_tx, medium_fee);

    auto empty_block = std::make_shared<Block>(0, "", "", 0, 24, 0, std::vector<std::shared_ptr<Tx>>{});
    auto assembled = Mempool::select_from_mempool(empty_block);

    ASSERT_EQ(assembled->txs.size(), 3);

    int parent_pos = -1, child_pos = -1, medium_pos = -1;
    for (size_t i = 0; i < assembled->txs.size(); i++)
    {
        const auto& id = assembled->txs[i]->id();
        if (id == parent_tx->id()) parent_pos = static_cast<int>(i);
        else if (id == child_tx->id()) child_pos = static_cast<int>(i);
        else if (id == medium_tx->id()) medium_pos = static_cast<int>(i);
    }

    EXPECT_NE(parent_pos, -1);
    EXPECT_NE(child_pos, -1);
    EXPECT_NE(medium_pos, -1);

    EXPECT_LT(parent_pos, child_pos);

    EXPECT_LT(child_pos, medium_pos);
}

TEST_F(CPFPTest, IndependentTxsSortedByFeeRate)
{
    auto tx_high = make_tx("utxo_high", 0, 500000, 400000);
    auto tx_mid = make_tx("utxo_mid", 0, 500000, 450000);
    auto tx_low = make_tx("utxo_low", 0, 500000, 495000);

    insert_into_mempool(tx_high, 100000);
    insert_into_mempool(tx_mid, 50000);
    insert_into_mempool(tx_low, 5000);

    auto empty_block = std::make_shared<Block>(0, "", "", 0, 24, 0, std::vector<std::shared_ptr<Tx>>{});
    auto assembled = Mempool::select_from_mempool(empty_block);

    ASSERT_EQ(assembled->txs.size(), 3);

    int high_pos = -1, mid_pos = -1, low_pos = -1;
    for (size_t i = 0; i < assembled->txs.size(); i++)
    {
        const auto& id = assembled->txs[i]->id();
        if (id == tx_high->id()) high_pos = static_cast<int>(i);
        else if (id == tx_mid->id()) mid_pos = static_cast<int>(i);
        else if (id == tx_low->id()) low_pos = static_cast<int>(i);
    }

    EXPECT_LT(high_pos, mid_pos);
    EXPECT_LT(mid_pos, low_pos);
}

TEST_F(CPFPTest, DeepAncestryPackage)
{
    auto gp_tx = make_tx("utxo_gp", 0, 1000000, 999900);
    insert_into_mempool(gp_tx, 100);

    auto parent_outpoint = std::make_shared<TxOutPoint>(gp_tx->id(), 0);
    auto parent_tin = std::make_shared<TxIn>(parent_outpoint, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
    auto parent_tout = std::make_shared<TxOut>(999800, "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs");
    auto parent_tx = std::make_shared<Tx>(std::vector{ parent_tin }, std::vector{ parent_tout }, 0);
    insert_into_mempool(parent_tx, 100);

    auto child_outpoint = std::make_shared<TxOutPoint>(parent_tx->id(), 0);
    auto child_tin = std::make_shared<TxIn>(child_outpoint, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
    auto child_tout = std::make_shared<TxOut>(800000, "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs");
    auto child_tx = std::make_shared<Tx>(std::vector{ child_tin }, std::vector{ child_tout }, 0);
    insert_into_mempool(child_tx, 199800);

    auto unrelated_tx = make_tx("utxo_unrelated", 0, 500000, 480000);
    insert_into_mempool(unrelated_tx, 20000);

    auto empty_block = std::make_shared<Block>(0, "", "", 0, 24, 0, std::vector<std::shared_ptr<Tx>>{});
    auto assembled = Mempool::select_from_mempool(empty_block);

    ASSERT_EQ(assembled->txs.size(), 4);

    int gp_pos = -1, parent_pos = -1, child_pos = -1, unrelated_pos = -1;
    for (size_t i = 0; i < assembled->txs.size(); i++)
    {
        const auto& id = assembled->txs[i]->id();
        if (id == gp_tx->id()) gp_pos = static_cast<int>(i);
        else if (id == parent_tx->id()) parent_pos = static_cast<int>(i);
        else if (id == child_tx->id()) child_pos = static_cast<int>(i);
        else if (id == unrelated_tx->id()) unrelated_pos = static_cast<int>(i);
    }

    EXPECT_LT(gp_pos, parent_pos);
    EXPECT_LT(parent_pos, child_pos);

    EXPECT_LT(child_pos, unrelated_pos);
}

TEST_F(CPFPTest, EmptyMempoolProducesEmptyBlock)
{
    auto empty_block = std::make_shared<Block>(0, "", "", 0, 24, 0, std::vector<std::shared_ptr<Tx>>{});
    auto assembled = Mempool::select_from_mempool(empty_block);

    EXPECT_TRUE(assembled->txs.empty());
}


