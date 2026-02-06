#include <memory>
#include <ranges>
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
#include "crypto/ecdsa.hpp"
#include "mining/pow.hpp"
#include "util/exceptions.hpp"
#include "util/utils.hpp"
#include "wallet/wallet.hpp"
#include <gtest/gtest.h>

TEST(RBFTest, Signaling)
{
    auto make_tx = [](std::vector<int32_t> seqs)
    {
        std::vector<std::shared_ptr<TxIn>> ins;
        for (auto s : seqs)
            ins.push_back(std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), s));
        auto tx_out = std::make_shared<TxOut>(1000, "addr");
        return std::make_shared<Tx>(ins, std::vector{ tx_out }, 0);
    };

    EXPECT_FALSE(make_tx({ TxIn::SEQUENCE_FINAL })->signals_rbf());
    EXPECT_TRUE(make_tx({ TxIn::SEQUENCE_RBF })->signals_rbf());
    EXPECT_TRUE(make_tx({ 0 })->signals_rbf());
    EXPECT_TRUE(make_tx({ TxIn::SEQUENCE_FINAL, TxIn::SEQUENCE_RBF })->signals_rbf());
}

#ifdef NDEBUG
TEST(RBFTest_LongRunning, MempoolReplacement)
{
    auto setup = []()
    {
        Chain::reset();
        Chain::connect_block(std::make_shared<Block>(*Chain::genesis_block));

        auto priv_key = Utils::hex_string_to_byte_array(
            "18e14a7b6a307f426a94f8114701e7c8e774e7f9a47e2c2035db29a206321725");
        auto pub_key = ECDSA::get_pub_key_from_priv_key(priv_key);
        auto address = Wallet::pub_key_to_address(pub_key);

        for (int i = 0; i < NetParams::COINBASE_MATURITY + 1; i++)
        {
            auto block = PoW::assemble_and_solve_block(address);
            Chain::connect_block(block);
        }

        std::shared_ptr<UTXO> utxo;
        {
            std::scoped_lock lock(UTXO::mutex);
            for (const auto& [_, u] : UTXO::map)
            {
                if (u->tx_out->to_address == address &&
                    (!u->is_coinbase ||
                        Chain::get_current_height() - u->height >= NetParams::COINBASE_MATURITY))
                {
                    utxo = u;
                    break;
                }
            }
        }
        return std::tuple{ priv_key, pub_key, address, utxo };
    };

    auto build_tx = [](const auto& priv_key, const auto& pub_key,
        const auto& outpoint, uint64_t out_value,
        const std::string& addr, int32_t seq)
    {
        auto tx_out = std::make_shared<TxOut>(out_value, addr);
        std::vector outs{ tx_out };
        auto tx_in = Wallet::build_tx_in(priv_key, pub_key, outpoint, outs, seq);
        return std::make_shared<Tx>(std::vector{ tx_in }, outs, 0);
    };

    {
        auto [priv_key, pub_key, address, utxo] = setup();
        ASSERT_NE(utxo, nullptr);
        const uint64_t val = utxo->tx_out->value;

        auto original = build_tx(priv_key, pub_key, utxo->tx_out_point,
            val - 1000, address, TxIn::SEQUENCE_RBF);
        Mempool::add_tx_to_mempool(original);
        ASSERT_TRUE(Mempool::map.contains(original->id()));

        auto replacement = build_tx(priv_key, pub_key, utxo->tx_out_point,
            val - 5000, address, TxIn::SEQUENCE_RBF);
        Mempool::add_tx_to_mempool(replacement);

        EXPECT_TRUE(Mempool::map.contains(replacement->id()));
        EXPECT_FALSE(Mempool::map.contains(original->id()));
    }

    {
        auto [priv_key, pub_key, address, utxo] = setup();
        ASSERT_NE(utxo, nullptr);
        const uint64_t val = utxo->tx_out->value;

        auto original = build_tx(priv_key, pub_key, utxo->tx_out_point,
            val - 5000, address, TxIn::SEQUENCE_RBF);
        Mempool::add_tx_to_mempool(original);
        ASSERT_TRUE(Mempool::map.contains(original->id()));

        auto replacement = build_tx(priv_key, pub_key, utxo->tx_out_point,
            val - 1000, address, TxIn::SEQUENCE_RBF);
        Mempool::add_tx_to_mempool(replacement);

        EXPECT_TRUE(Mempool::map.contains(original->id()));
        EXPECT_FALSE(Mempool::map.contains(replacement->id()));
    }

    {
        auto [priv_key, pub_key, address, utxo] = setup();
        ASSERT_NE(utxo, nullptr);
        const uint64_t val = utxo->tx_out->value;

        auto original = build_tx(priv_key, pub_key, utxo->tx_out_point,
            val - 1000, address, TxIn::SEQUENCE_FINAL);
        EXPECT_FALSE(original->signals_rbf());
        Mempool::add_tx_to_mempool(original);
        ASSERT_TRUE(Mempool::map.contains(original->id()));

        auto replacement = build_tx(priv_key, pub_key, utxo->tx_out_point,
            val - 5000, address, TxIn::SEQUENCE_RBF);
        Mempool::add_tx_to_mempool(replacement);

        EXPECT_TRUE(Mempool::map.contains(original->id()));
        EXPECT_FALSE(Mempool::map.contains(replacement->id()));
    }

    {
        auto [priv_key, pub_key, address, utxo] = setup();
        ASSERT_NE(utxo, nullptr);
        const uint64_t val = utxo->tx_out->value;

        auto parent = build_tx(priv_key, pub_key, utxo->tx_out_point,
            val - 2000, address, TxIn::SEQUENCE_RBF);
        Mempool::add_tx_to_mempool(parent);
        ASSERT_TRUE(Mempool::map.contains(parent->id()));

        auto child_outpoint = std::make_shared<TxOutPoint>(parent->id(), 0);
        auto child = build_tx(priv_key, pub_key, child_outpoint,
            val - 3000, address, TxIn::SEQUENCE_RBF);
        Mempool::add_tx_to_mempool(child);
        ASSERT_TRUE(Mempool::map.contains(child->id()));

        auto replacement = build_tx(priv_key, pub_key, utxo->tx_out_point,
            val - 10000, address, TxIn::SEQUENCE_RBF);
        Mempool::add_tx_to_mempool(replacement);

        EXPECT_TRUE(Mempool::map.contains(replacement->id()));
        EXPECT_FALSE(Mempool::map.contains(parent->id()));
        EXPECT_FALSE(Mempool::map.contains(child->id()));
    }
}
#endif
