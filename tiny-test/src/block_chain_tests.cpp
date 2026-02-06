#include <array>
#include <memory>
#include <ranges>
#include <vector>

#include "core/block.hpp"
#include "core/chain.hpp"
#include "crypto/ecdsa.hpp"
#include "util/exceptions.hpp"
#include "core/mempool.hpp"
#include "core/net_params.hpp"
#include "mining/pow.hpp"
#include "core/tx.hpp"
#include "core/tx_out.hpp"
#include "core/unspent_tx_out.hpp"
#include "util/utils.hpp"
#include "wallet/wallet.hpp"
#include <gtest/gtest.h>

TEST(BlockChainTest, MedianTimePast)
{
	Chain::active_chain.clear();

	EXPECT_EQ(0, Chain::get_median_time_past(10));

	const std::array<int64_t, 5> timestamps{ 1, 30, 60, 90, 400 };

	for (auto timestamp : timestamps)
	{
		auto dummy_block = std::make_shared<Block>(0, "foo", "foo", timestamp, 1, 0, std::vector<std::shared_ptr<Tx>>());

		Chain::active_chain.push_back(dummy_block);
	}

	EXPECT_EQ(400, Chain::get_median_time_past(1));
	EXPECT_EQ(90, Chain::get_median_time_past(3));
	EXPECT_EQ(400, Chain::get_median_time_past(2));
	EXPECT_EQ(60, Chain::get_median_time_past(5));
}

const auto chain1_block1_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs")
		}, 0)
};
const auto chain1_block1 = std::make_shared<Block>(
	0, "", "67cf53bf61ce6af428186e7c64e89dc90b2de39fe5b0cf2392d6e9b447b90f9b",
	1501821412, 24, 6148914691237377386ULL, chain1_block1_txs);
const auto chain1_block2_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, 0)
};
const auto chain1_block2 = std::make_shared<Block>(
	0, "0000000e5425335e778b242aa8a1e8b5b7be97792f63cf725f475500198bb0aa",
	"6cf00fa41b5abbba819ddd167ed2fb53cc72dd5ca6d77f15588260919bb6678c",
	1501826444, 24, 10248191152064341128ULL, chain1_block2_txs);
const auto chain1_block3_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, 0)
};
const auto chain1_block3 = std::make_shared<Block>(
	0, "00000029c69f503c15589ef2f1bfdf0b392d11ad499081bb6ec74deda8251a54",
	"6cf00fa41b5abbba819ddd167ed2fb53cc72dd5ca6d77f15588260919bb6678c",
	1501826556, 24, 6148914691236623873ULL, chain1_block3_txs);
const auto chain1 = std::vector{ chain1_block1, chain1_block2, chain1_block3 };

const auto chain2_block2_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, 0)
};
const auto chain2_block2 = std::make_shared<Block>(
	0, "0000000e5425335e778b242aa8a1e8b5b7be97792f63cf725f475500198bb0aa",
	"6cf00fa41b5abbba819ddd167ed2fb53cc72dd5ca6d77f15588260919bb6678c",
	1501826757, 24, 12297829382473771619ULL, chain2_block2_txs);
const auto chain2_block3_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, 0)
};
const auto chain2_block3 = std::make_shared<Block>(
	0, "0000007604573a05541db73d866966729025ea1061028f22f83914d117048562",
	"6cf00fa41b5abbba819ddd167ed2fb53cc72dd5ca6d77f15588260919bb6678c",
	1501826872, 24, 12297829382484582385ULL, chain2_block3_txs);
const auto chain2_block4_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, 0)
};
const auto chain2_block4 = std::make_shared<Block>(
	0, "00000049f7ed5925e1dfafe24499787f6ed55d0b86bfc810c6c9f5bbe98e6903",
	"6cf00fa41b5abbba819ddd167ed2fb53cc72dd5ca6d77f15588260919bb6678c",
	1501826949, 24, 2244129ULL, chain2_block4_txs);
const auto chain2_block5_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, 0)
};
const auto chain2_block5 = std::make_shared<Block>(
	0, "0000003abefd09cd37881e0ded0441af3499586c8a5d587995ef32fc14b78514",
	"6cf00fa41b5abbba819ddd167ed2fb53cc72dd5ca6d77f15588260919bb6678c",
	1501827000, 24, 4099276460824672001ULL, chain2_block5_txs);
const auto chain2 = std::vector{
	chain1_block1, chain2_block2, chain2_block3, chain2_block4, chain2_block5
};

TEST(BlockChainTest, Reorg)
{
	Chain::reset();

	for (const auto& block : chain1)
		ASSERT_EQ(Chain::ACTIVE_CHAIN_IDX, Chain::connect_block(block));

	Chain::side_branches.clear();
	Mempool::map.clear();
	UTXO::map.clear();

	for (const auto& block : Chain::active_chain)
	{
		for (const auto& tx : block->txs)
		{
			for (uint32_t i = 0; i < tx->tx_outs.size(); i++)
			{
				UTXO::add_to_map(tx->tx_outs[i], tx->id(), i, tx->is_coinbase(), Chain::active_chain.size());
			}
		}
	}

	ASSERT_EQ(2, UTXO::map.size());
	ASSERT_FALSE(Chain::reorg_if_necessary());

	ASSERT_EQ(1, Chain::connect_block(chain2[1]));

	ASSERT_FALSE(Chain::reorg_if_necessary());
	ASSERT_TRUE(Chain::side_branches.size() == 1);
	ASSERT_EQ(*chain2[1], *Chain::side_branches[0][0]);
	ASSERT_EQ(chain1.size(), Chain::active_chain.size());
	for (uint32_t i = 0; i < Chain::active_chain.size(); i++)
	{
		ASSERT_EQ(*chain1[i], *Chain::active_chain[i]);
	}
	ASSERT_TRUE(Mempool::map.empty());
	const std::array<std::string, 2> tx_ids{ "b6678c", "b90f9b" };
	ASSERT_EQ(tx_ids.size(), UTXO::map.size());
	for (const auto& k : UTXO::map | std::views::keys)
	{
		bool found = false;
		for (const auto& tx_id : tx_ids)
		{
			found |= k->tx_id.ends_with(tx_id);
			if (found)
			{
				break;
			}
		}
		ASSERT_TRUE(found);
	}

	ASSERT_EQ(1, Chain::connect_block(chain2[2]));

	ASSERT_FALSE(Chain::reorg_if_necessary());
	ASSERT_EQ(1, Chain::side_branches.size());
	std::array side_branch_test{ chain2[1], chain2[2] };
	for (uint32_t i = 0; i < Chain::side_branches[0].size(); i++)
	{
		ASSERT_EQ(*side_branch_test[i], *Chain::side_branches[0][i]);
	}
	ASSERT_EQ(Chain::active_chain.size(), chain1.size());
	for (uint32_t i = 0; i < Chain::active_chain.size(); i++)
	{
		ASSERT_EQ(*chain1[i], *Chain::active_chain[i]);
	}
	ASSERT_TRUE(Mempool::map.empty());
	ASSERT_EQ(tx_ids.size(), UTXO::map.size());
	for (const auto& k : UTXO::map | std::views::keys)
	{
		bool found = false;
		for (const auto& tx_id : tx_ids)
		{
			found |= k->tx_id.ends_with(tx_id);
			if (found)
			{
				break;
			}
		}
		ASSERT_TRUE(found);
	}

	auto chain3_faulty = chain2;
	auto chain2_block4_copy = std::make_shared<Block>(*chain2_block4);
	chain2_block4_copy->nonce = 1;
	chain3_faulty[3] = chain2_block4_copy;

	ASSERT_EQ(-1, Chain::connect_block(chain3_faulty[3]));
	ASSERT_FALSE(Chain::reorg_if_necessary());

	ASSERT_EQ(1, Chain::side_branches.size());
	for (uint32_t i = 0; i < Chain::side_branches[0].size(); i++)
	{
		ASSERT_EQ(*side_branch_test[i], *Chain::side_branches[0][i]);
	}
	ASSERT_EQ(Chain::active_chain.size(), chain1.size());
	for (uint32_t i = 0; i < Chain::active_chain.size(); i++)
	{
		ASSERT_EQ(*chain1[i], *Chain::active_chain[i]);
	}
	ASSERT_TRUE(Mempool::map.empty());
	ASSERT_EQ(tx_ids.size(), UTXO::map.size());
	for (const auto& k : UTXO::map | std::views::keys)
	{
		bool found = false;
		for (const auto& tx_id : tx_ids)
		{
			found |= k->tx_id.ends_with(tx_id);
			if (found)
			{
				break;
			}
		}
		ASSERT_TRUE(found);
	}

	ASSERT_EQ(1, Chain::connect_block(chain2[3]));
	ASSERT_EQ(1, Chain::connect_block(chain2[4]));

	ASSERT_EQ(1, Chain::side_branches.size());
	ASSERT_EQ(2, Chain::side_branches[0].size());
	std::vector<std::string> side_branch_ids;
	side_branch_ids.reserve(Chain::side_branches[0].size());
	for (const auto& block : Chain::side_branches[0])
	{
		side_branch_ids.emplace_back(block->id());
	}
	std::vector<std::string> chain1_ids;
	chain1_ids.reserve(chain1.size());
	for (uint32_t i = 1; i < chain1.size(); i++)
	{
		chain1_ids.emplace_back(chain1[i]->id());
	}
	ASSERT_EQ(chain1_ids, side_branch_ids);
	std::array side_branch_test2{ chain1[1], chain1[2] };
	for (uint32_t i = 0; i < Chain::side_branches[0].size(); i++)
	{
		ASSERT_EQ(*side_branch_test2[i], *Chain::side_branches[0][i]);
	}
	ASSERT_TRUE(Mempool::map.empty());
	const std::array<std::string, 2> tx_ids2{ "b90f9b", "b6678c" };
	ASSERT_EQ(UTXO::map.size(), tx_ids2.size());
	for (const auto& k : UTXO::map | std::views::keys)
	{
		bool found = false;
		for (const auto& tx_id : tx_ids2)
		{
			found |= k->tx_id.ends_with(tx_id);
			if (found)
			{
				break;
			}
		}
		ASSERT_TRUE(found);
	}
}

#ifdef NDEBUG
TEST(BlockChainTest_LongRunning, DependentTxsInSingleBlock)
{
	Chain::reset();

	ASSERT_EQ(Chain::ACTIVE_CHAIN_IDX, Chain::connect_block(chain1[0]));
	ASSERT_EQ(Chain::ACTIVE_CHAIN_IDX, Chain::connect_block(chain1[1]));

	ASSERT_EQ(2, Chain::active_chain.size());
	ASSERT_EQ(2, UTXO::map.size());

	auto priv_key = Utils::hex_string_to_byte_array("18e14a7b6a307f426a94f8114701e7c8e774e7f9a47e2c2035db29a206321725");
	auto pub_key = ECDSA::get_pub_key_from_priv_key(priv_key);
	auto address = Wallet::pub_key_to_address(pub_key);

	auto utxo_it = std::ranges::find_if(UTXO::map,
		[&address](const std::pair<const std::shared_ptr<TxOutPoint>, std::shared_ptr<UTXO>>& p)
	{
		return p.second->tx_out->to_address == address;
	});
	ASSERT_NE(utxo_it, UTXO::map.end());
	const auto& utxo1 = utxo_it->second;
	auto tx_out1 = std::make_shared<TxOut>(901, utxo1->tx_out->to_address);
	std::vector tx_outs1{ tx_out1 };
	auto tx_in1 = Wallet::build_tx_in(priv_key, pub_key, utxo1->tx_out_point, tx_outs1);
	auto tx1 = std::make_shared<Tx>(std::vector{ tx_in1 }, tx_outs1, 0);

	ASSERT_THROW(
		{
		try
		{
		tx1->validate(Tx::ValidateRequest());
		}
		catch (const TxValidationException& ex)
		{
		ASSERT_STREQ("Coinbase UTXO not ready for spending", ex.what());
		throw;
		}
		},
		TxValidationException);

	Chain::connect_block(chain1[2]);

	Mempool::add_tx_to_mempool(tx1);
	ASSERT_TRUE(Mempool::map.contains(tx1->id()));

	auto tx_out2 = std::make_shared<TxOut>(9001, tx_out1->to_address);
	std::vector tx_outs2{ tx_out2 };
	auto tx_out_point2 = std::make_shared<TxOutPoint>(tx1->id(), 0);
	auto tx_in2 = Wallet::build_tx_in(priv_key, pub_key, tx_out_point2, tx_outs2);
	auto tx2 = std::make_shared<Tx>(std::vector{ tx_in2 }, tx_outs2, 0);

	Mempool::add_tx_to_mempool(tx2);
	ASSERT_FALSE(Mempool::map.contains(tx2->id()));

	ASSERT_THROW(
		{
		try
		{
		tx2->validate(Tx::ValidateRequest());
		}
		catch (const TxValidationException& ex)
		{
		ASSERT_STREQ("Spent value more than available", ex.what());
		throw;
		}
		},
		TxValidationException);

	tx_out2->value = 901;
	tx_in2 = Wallet::build_tx_in(priv_key, pub_key, tx_out_point2, tx_outs2);
	tx2->tx_ins[0] = tx_in2;

	Mempool::add_tx_to_mempool(tx2);
	ASSERT_TRUE(Mempool::map.contains(tx2->id()));

	auto block = PoW::assemble_and_solve_block(address);

	ASSERT_EQ(Chain::ACTIVE_CHAIN_IDX, Chain::connect_block(block));

	ASSERT_EQ(*Chain::active_chain.back(), *block);
	ASSERT_EQ(2, block->txs.size() - 1);
	std::array txs{ tx1, tx2 };
	for (uint32_t i = 0; i < txs.size(); i++)
	{
		ASSERT_EQ(*txs[i], *block->txs[i + 1]);
	}
	ASSERT_FALSE(Mempool::map.contains(tx1->id()));
	ASSERT_FALSE(Mempool::map.contains(tx2->id()));
	auto map_it1 = std::ranges::find_if(UTXO::map,
		[&tx1](const std::pair<const std::shared_ptr<TxOutPoint>, std::shared_ptr<UTXO>>& p)
	{
		const auto& [tx_out_point, utxo] = p;
		return tx_out_point->tx_id == tx1->id() && tx_out_point->tx_out_idx == 0;
	});
	ASSERT_EQ(map_it1, UTXO::map.end());
	auto map_it2 = std::ranges::find_if(UTXO::map,
		[&tx2](const std::pair<const std::shared_ptr<TxOutPoint>, std::shared_ptr<UTXO>>& p)
	{
		const auto& [tx_out_point, utxo] = p;
		return tx_out_point->tx_id == tx2->id() && tx_out_point->tx_out_idx == 0;
	});
	ASSERT_NE(map_it2, UTXO::map.end());
}

TEST(BlockChainTest_LongRunning, MinerTransaction)
{
	Chain::reset();

	const auto [miner_priv_key, miner_pub_key, miner_address] = Wallet::init_wallet("miner.dat");
	const auto [receiver_priv_key, receiver_pub_key, receiver_address] = Wallet::init_wallet("receiver.dat");

	const auto first_block = PoW::assemble_and_solve_block(miner_address);
	if (first_block == nullptr)
		FAIL();
	Chain::connect_block(first_block);
	Chain::save_to_disk();

	for (int i = 0; i < NetParams::COINBASE_MATURITY + 1; i++)
	{
		const auto maturity_blocks = PoW::assemble_and_solve_block(miner_address);
		if (maturity_blocks == nullptr)
			FAIL();
		Chain::connect_block(maturity_blocks);
		Chain::save_to_disk();
	}

	ASSERT_GT(Wallet::get_balance_miner(miner_address), 0);
	auto tx = Wallet::send_value_miner(first_block->txs.front()->tx_outs.front()->value / 2, 100, receiver_address,
		miner_priv_key);
	ASSERT_TRUE(tx != nullptr);
	ASSERT_EQ(Wallet::get_tx_status_miner(tx->id()).status, TxStatus::Mempool);

	const auto post_tx_block = PoW::assemble_and_solve_block(miner_address);
	if (post_tx_block == nullptr)
		FAIL();
	Chain::connect_block(post_tx_block);
	Chain::save_to_disk();

	auto mined_tx_status = Wallet::get_tx_status_miner(tx->id());
	ASSERT_EQ(mined_tx_status.status, TxStatus::Mined);
	ASSERT_EQ(mined_tx_status.block_id, post_tx_block->id());
	ASSERT_GT(Wallet::get_balance_miner(receiver_address), 0);
}
#endif
