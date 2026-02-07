#include <array>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

#include "core/block.hpp"
#include "core/chain.hpp"
#include "crypto/ecdsa.hpp"
#include "util/binary_buffer.hpp"
#include "util/exceptions.hpp"
#include "core/mempool.hpp"
#include "core/net_params.hpp"
#include "mining/merkle_tree.hpp"
#include "mining/pow.hpp"
#include "core/tx.hpp"
#include "core/tx_in.hpp"
#include "core/tx_out.hpp"
#include "core/tx_out_point.hpp"
#include "core/unspent_tx_out.hpp"
#include "util/utils.hpp"
#include "wallet/wallet.hpp"
#include <gtest/gtest.h>

class BlockChainTest : public ::testing::Test
{
protected:
	void SetUp() override { Chain::reset(); }
	void TearDown() override { Chain::reset(); }
};

TEST_F(BlockChainTest, MedianTimePast)
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

	EXPECT_EQ(60, Chain::get_median_time_past_at_height(4, 5));
	EXPECT_EQ(30, Chain::get_median_time_past_at_height(2, 3));
	EXPECT_EQ(1, Chain::get_median_time_past_at_height(0, 11));
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

TEST_F(BlockChainTest, Reorg)
{
	for (const auto& block : chain1)
		ASSERT_EQ(Chain::ACTIVE_CHAIN_IDX, Chain::connect_block(block));

	Chain::side_branches.clear();
	Mempool::map.clear();
	Mempool::total_size_bytes = 0;
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
	ASSERT_EQ(Chain::ACTIVE_CHAIN_IDX, Chain::connect_block(chain2[4]));

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
TEST_F(BlockChainTest, DependentTxsInSingleBlock)
{
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

TEST_F(BlockChainTest, MinerTransaction)
{
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

TEST(LockTimeTest, IsFinalAndCheckLockTime)
{
	auto make_tx = [](int32_t seq, int64_t lock_time)
	{
		auto tx_in = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), seq);
		auto tx_out = std::make_shared<TxOut>(1000, "addr");
		return std::make_shared<Tx>(std::vector{ tx_in }, std::vector{ tx_out }, lock_time);
	};

	EXPECT_TRUE(make_tx(0, 0)->is_final());
	EXPECT_TRUE(make_tx(TxIn::SEQUENCE_FINAL, 100)->is_final());
	EXPECT_FALSE(make_tx(0, 100)->is_final());

	EXPECT_THROW(make_tx(0, 500)->check_lock_time(499, 0), TxValidationException);
	EXPECT_NO_THROW(make_tx(0, 500)->check_lock_time(500, 0));

	const int64_t lock_ts = NetParams::LOCKTIME_THRESHOLD + 1000;
	EXPECT_THROW(make_tx(0, lock_ts)->check_lock_time(0, lock_ts - 1), TxValidationException);
	EXPECT_NO_THROW(make_tx(0, lock_ts)->check_lock_time(0, lock_ts));

	EXPECT_NO_THROW(make_tx(TxIn::SEQUENCE_FINAL, 999999)->check_lock_time(1, 0));
}

TEST_F(BlockChainTest, CheckSequenceLocks)
{
	const auto& genesis_tx = Chain::genesis_tx;
	const auto genesis_tx_id = genesis_tx->id();

	UTXO::add_to_map(genesis_tx->tx_outs[0], genesis_tx_id, 0, true, 1);

	for (int i = 0; i < 5; i++)
	{
		auto blk = std::make_shared<Block>(0, "prev", "merkle",
			1501821412 + (i + 1) * 600, 24, 0, std::vector<std::shared_ptr<Tx>>());
		Chain::active_chain.push_back(blk);
	}

	auto to_spend_blk = std::make_shared<TxOutPoint>(genesis_tx_id, 0);

	auto make_tx_with_seq = [](const std::shared_ptr<TxOutPoint>& outpoint, int32_t seq)
	{
		auto tx_in = std::make_shared<TxIn>(outpoint, std::vector<uint8_t>(), std::vector<uint8_t>(), seq);
		auto tx_out = std::make_shared<TxOut>(1000, "addr");
		return std::make_shared<Tx>(std::vector{ tx_in }, std::vector{ tx_out }, 0);
	};

	EXPECT_NO_THROW(make_tx_with_seq(to_spend_blk, TxIn::encode_relative_blocks(3))->check_sequence_locks(6, 0));
	EXPECT_THROW(make_tx_with_seq(to_spend_blk, TxIn::encode_relative_blocks(10))->check_sequence_locks(6, 0), TxValidationException);
	EXPECT_NO_THROW(make_tx_with_seq(to_spend_blk, TxIn::SEQUENCE_FINAL)->check_sequence_locks(1, 0));

	UTXO::remove_from_map(genesis_tx_id, 0);

	Chain::active_chain.clear();

	for (int i = 0; i < 12; i++)
	{
		auto blk = std::make_shared<Block>(0, "prev", "merkle",
			static_cast<int64_t>(i) * 600, 24, 0, std::vector<std::shared_ptr<Tx>>());
		Chain::active_chain.push_back(blk);
	}

	const std::string tx_id = "abc123";
	UTXO::add_to_map(std::make_shared<TxOut>(5000, "addr"), tx_id, 0, false, 2);

	auto to_spend_time = std::make_shared<TxOutPoint>(tx_id, 0);

	EXPECT_NO_THROW(make_tx_with_seq(to_spend_time, TxIn::encode_relative_time(5))->check_sequence_locks(12, 3600));
	EXPECT_THROW(make_tx_with_seq(to_spend_time, TxIn::encode_relative_time(100))->check_sequence_locks(12, 3600), TxValidationException);

	UTXO::remove_from_map(tx_id, 0);
}

TEST_F(BlockChainTest, OrphanStorageAndResolution)
{
	ASSERT_EQ(Chain::ACTIVE_CHAIN_IDX, Chain::connect_block(chain1_block1));

	ASSERT_EQ(-1, Chain::connect_block(chain1_block3));
	ASSERT_EQ(1u, Chain::orphan_blocks.size());
	ASSERT_EQ(chain1_block3->prev_block_hash, Chain::orphan_blocks.begin()->first);
	ASSERT_EQ(chain1_block3->id(), Chain::orphan_blocks.begin()->second.block->id());

	ASSERT_EQ(-1, Chain::connect_block(chain1_block3));
	ASSERT_EQ(1u, Chain::orphan_blocks.size());

	ASSERT_EQ(Chain::ACTIVE_CHAIN_IDX, Chain::connect_block(chain1_block2));

	ASSERT_TRUE(Chain::orphan_blocks.empty());
	ASSERT_EQ(3u, Chain::active_chain.size());
	ASSERT_EQ(chain1_block3->id(), Chain::active_chain.back()->id());
}

TEST_F(BlockChainTest, OrphanPoolSizeLimit)
{
	for (uint32_t i = 0; i < NetParams::MAX_ORPHAN_BLOCKS; i++)
	{
		auto dummy = std::make_shared<Block>(
			0, "nonexistent_parent_" + std::to_string(i), "merkle",
			1501821412 + i, 24, i, std::vector{ chain1_block1_txs[0] });

		Chain::orphan_blocks.emplace("nonexistent_parent_" + std::to_string(i),
			OrphanBlock{ dummy, static_cast<int64_t>(1501821412 + i) });
	}

	ASSERT_EQ(NetParams::MAX_ORPHAN_BLOCKS, Chain::orphan_blocks.size());

	ASSERT_EQ(Chain::ACTIVE_CHAIN_IDX, Chain::connect_block(chain1_block1));
	ASSERT_EQ(-1, Chain::connect_block(chain1_block3));

	ASSERT_LE(Chain::orphan_blocks.size(), static_cast<size_t>(NetParams::MAX_ORPHAN_BLOCKS));

	auto range = Chain::orphan_blocks.equal_range(chain1_block3->prev_block_hash);
	bool found = false;
	for (auto it = range.first; it != range.second; ++it)
	{
		if (it->second.block->id() == chain1_block3->id())
		{
			found = true;
			break;
		}
	}
	ASSERT_TRUE(found);
}

TEST_F(BlockChainTest, RBFSignaling)
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
TEST_F(BlockChainTest, RBFMempoolReplacement)
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

TEST(TxValidationTest, DuplicateInputRejection)
{
	auto outpoint = std::make_shared<TxOutPoint>("deadbeef", 0);
	auto tx_in1 = std::make_shared<TxIn>(outpoint, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
	auto tx_in2 = std::make_shared<TxIn>(outpoint, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
	auto tx_out = std::make_shared<TxOut>(1000, "addr");
	auto tx = std::make_shared<Tx>(std::vector{ tx_in1, tx_in2 }, std::vector{ tx_out }, 0);

	EXPECT_THROW(
		{
			try
			{
				tx->validate_basics();
			}
			catch (const TxValidationException& ex)
			{
				EXPECT_STREQ("Duplicate input", ex.what());
				throw;
			}
		},
		TxValidationException);

	auto outpoint2 = std::make_shared<TxOutPoint>("deadbeef", 1);
	auto tx_in3 = std::make_shared<TxIn>(outpoint, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
	auto tx_in4 = std::make_shared<TxIn>(outpoint2, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
	auto tx2 = std::make_shared<Tx>(std::vector{ tx_in3, tx_in4 }, std::vector{ tx_out }, 0);
	EXPECT_NO_THROW(tx2->validate_basics());
}

TEST_F(BlockChainTest, DuplicateTxIdInBlock)
{
	auto dup_tx = std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs")
		}, 0);

	auto block = std::make_shared<Block>(
		0, "", "merkle", 1501821412, 24, 0,
		std::vector{ dup_tx, dup_tx });
	block->merkle_hash = MerkleTree::get_root_of_txs(block->txs)->value;

	EXPECT_THROW(
		{
			try
			{
				Chain::validate_block(block);
			}
			catch (const BlockValidationException& ex)
			{
				std::string msg = ex.what();
				EXPECT_TRUE(msg.find("Duplicate transaction") != std::string::npos);
				throw;
			}
		},
		BlockValidationException);
}

#ifdef NDEBUG
TEST_F(BlockChainTest, CoinbaseSubsidyValidation)
{
	auto priv_key = Utils::hex_string_to_byte_array(
		"18e14a7b6a307f426a94f8114701e7c8e774e7f9a47e2c2035db29a206321725");
	auto pub_key = ECDSA::get_pub_key_from_priv_key(priv_key);
	auto address = Wallet::pub_key_to_address(pub_key);

	for (int i = 0; i < NetParams::COINBASE_MATURITY + 2; i++)
	{
		auto block = PoW::assemble_and_solve_block(address);
		if (block == nullptr)
			FAIL();
		Chain::connect_block(block);
	}

	std::string prev_block_hash;
	{
		std::scoped_lock lock(Chain::mutex);
		prev_block_hash = Chain::active_chain.back()->id();
	}
	auto bits = PoW::get_next_work_required(prev_block_hash);

	auto inflated_coinbase = Tx::create_coinbase(address, 99999 * NetParams::COIN,
		Chain::get_current_height());
	auto inflated_block = std::make_shared<Block>(
		0, prev_block_hash, "", Utils::get_unix_timestamp(),
		bits, 0, std::vector{ inflated_coinbase });
	inflated_block->merkle_hash = MerkleTree::get_root_of_txs(inflated_block->txs)->value;

	auto mined = PoW::mine(inflated_block);
	if (mined != nullptr)
	{
		EXPECT_THROW(
			{
				try
				{
					Chain::validate_block(mined);
				}
				catch (const BlockValidationException& ex)
				{
					std::string msg = ex.what();
					EXPECT_TRUE(msg.find("Coinbase value") != std::string::npos);
					throw;
				}
			},
			BlockValidationException);
	}
}
#endif

TEST(TxValidationBasicsTest, EmptyOutputsRejected)
{
	auto to_spend = std::make_shared<TxOutPoint>("txid", 0);
	auto tx_in = std::make_shared<TxIn>(to_spend, std::vector<uint8_t>{}, std::vector<uint8_t>{}, -1);
	auto tx = std::make_shared<Tx>(std::vector{ tx_in }, std::vector<std::shared_ptr<TxOut>>{}, 0);

	EXPECT_THROW(tx->validate_basics(), TxValidationException);
}

TEST(TxValidationBasicsTest, EmptyInputsNonCoinbaseRejected)
{
	auto tx_out = std::make_shared<TxOut>(1000, "addr");
	auto tx = std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{}, std::vector{ tx_out }, 0);

	EXPECT_THROW(tx->validate_basics(false), TxValidationException);
}

TEST(TxValidationBasicsTest, EmptyInputsCoinbaseAllowed)
{
	auto tx_out = std::make_shared<TxOut>(1000, "addr");
	auto tx = std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{}, std::vector{ tx_out }, 0);

	EXPECT_NO_THROW(tx->validate_basics(true));
}

TEST(TxValidationBasicsTest, SingleOutputExceedingMaxMoney)
{
	auto tx_in = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{}, std::vector<uint8_t>{}, -1);
	auto tx_out = std::make_shared<TxOut>(NetParams::MAX_MONEY + 1, "addr");
	auto tx = std::make_shared<Tx>(std::vector{ tx_in }, std::vector{ tx_out }, 0);

	EXPECT_THROW(tx->validate_basics(true), TxValidationException);
}

TEST(TxValidationBasicsTest, TotalOutputsExceedingMaxMoney)
{
	auto tx_in = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{}, std::vector<uint8_t>{}, -1);
	const uint64_t half_plus_one = NetParams::MAX_MONEY / 2 + 1;
	auto tx_out1 = std::make_shared<TxOut>(half_plus_one, "addr1");
	auto tx_out2 = std::make_shared<TxOut>(half_plus_one, "addr2");
	auto tx = std::make_shared<Tx>(std::vector{ tx_in }, std::vector{ tx_out1, tx_out2 }, 0);

	EXPECT_THROW(tx->validate_basics(true), TxValidationException);
}

TEST(TxValidationBasicsTest, CreateCoinbaseStructure)
{
	auto coinbase = Tx::create_coinbase("1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs", 5000000000ULL, 100);

	EXPECT_TRUE(coinbase->is_coinbase());
	EXPECT_EQ(1, coinbase->tx_ins.size());
	EXPECT_EQ(nullptr, coinbase->tx_ins[0]->to_spend);
	EXPECT_EQ(1, coinbase->tx_outs.size());
	EXPECT_EQ(5000000000ULL, coinbase->tx_outs[0]->value);
	EXPECT_EQ("1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs", coinbase->tx_outs[0]->to_address);
	EXPECT_EQ(0, coinbase->lock_time);

	EXPECT_FALSE(coinbase->tx_ins[0]->unlock_sig.empty());

	BinaryBuffer buf(coinbase->tx_ins[0]->unlock_sig);
	int64_t decoded_height = 0;
	ASSERT_TRUE(buf.read(decoded_height));
	EXPECT_EQ(100, decoded_height);
}

TEST(TxValidationBasicsTest, IsCoinbaseDetection)
{
	auto tx_in_cb = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{}, std::vector<uint8_t>{}, -1);
	auto tx_out = std::make_shared<TxOut>(100, "addr");
	auto coinbase_tx = std::make_shared<Tx>(std::vector{ tx_in_cb }, std::vector{ tx_out }, 0);
	EXPECT_TRUE(coinbase_tx->is_coinbase());

	auto tx_in2 = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{}, std::vector<uint8_t>{}, -1);
	auto non_coinbase = std::make_shared<Tx>(std::vector{ tx_in_cb, tx_in2 }, std::vector{ tx_out }, 0);
	EXPECT_FALSE(non_coinbase->is_coinbase());

	auto outpoint = std::make_shared<TxOutPoint>("txid", 0);
	auto tx_in_normal = std::make_shared<TxIn>(outpoint, std::vector<uint8_t>{}, std::vector<uint8_t>{}, -1);
	auto normal_tx = std::make_shared<Tx>(std::vector{ tx_in_normal }, std::vector{ tx_out }, 0);
	EXPECT_FALSE(normal_tx->is_coinbase());
}
