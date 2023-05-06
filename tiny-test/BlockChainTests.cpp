#include "pch.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <ranges>
#include <vector>

#include "../tiny-lib/Block.hpp"
#include "../tiny-lib/Chain.hpp"
#include "../tiny-lib/ECDSA.hpp"
#include "../tiny-lib/Exceptions.hpp"
#include "../tiny-lib/Mempool.hpp"
#include "../tiny-lib/NetParams.hpp"
#include "../tiny-lib/PoW.hpp"
#include "../tiny-lib/Tx.hpp"
#include "../tiny-lib/TxOut.hpp"
#include "../tiny-lib/UnspentTxOut.hpp"
#include "../tiny-lib/Utils.hpp"
#include "../tiny-lib/Wallet.hpp"
#include "gtest/gtest.h"

TEST(BlockChainTest, MedianTimePast)
{
	Chain::ActiveChain.clear();

	EXPECT_EQ(0, Chain::GetMedianTimePast(10));

	const std::array<int64_t, 5> timestamps{ 1, 30, 60, 90, 400 };

	for (auto timestamp : timestamps)
	{
		auto dummy_block = std::make_shared<Block>(0, "foo", "foo", timestamp, 1, 0, std::vector<std::shared_ptr<Tx>>());

		Chain::ActiveChain.push_back(dummy_block);
	}

	EXPECT_EQ(400, Chain::GetMedianTimePast(1));
	EXPECT_EQ(90, Chain::GetMedianTimePast(3));
	EXPECT_EQ(90, Chain::GetMedianTimePast(2));
	EXPECT_EQ(60, Chain::GetMedianTimePast(5));
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
	0, "", "a4a241a0b693ad8ee736907fbe3fc572044b380cdc186f80647f1e8354bb2ba7",
	1501821412, 24, 5804256, chain1_block1_txs);
const auto chain1_block2_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, 0)
};
const auto chain1_block2 = std::make_shared<Block>(
	0, "0000001616129fcc8a1240972d1e39a8569c7db3e965e38db70ccd4418815efd",
	"8b0485e1a7823fc2ad3195837146e2b01e860a18f0cf81e524078b0116400430",
	1501826444, 24, 13835058055285570289, chain1_block2_txs);
const auto chain1_block3_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, 0)
};
const auto chain1_block3 = std::make_shared<Block>(
	0, "000000731ba1a0b651182140e8332287186c6a93ddbfc42455c3f88e020a5ce8",
	"8b0485e1a7823fc2ad3195837146e2b01e860a18f0cf81e524078b0116400430",
	1501826556, 24, 9223372036856676382, chain1_block3_txs);
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
	0, "0000001616129fcc8a1240972d1e39a8569c7db3e965e38db70ccd4418815efd",
	"8b0485e1a7823fc2ad3195837146e2b01e860a18f0cf81e524078b0116400430",
	1501826757, 24, 9223372036864717828, chain2_block2_txs);
const auto chain2_block3_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, 0)
};
const auto chain2_block3 = std::make_shared<Block>(
	0, "0000002def15eabb75ecde313f0f1e239592c758362b1f892f80e9c369a23bcc",
	"8b0485e1a7823fc2ad3195837146e2b01e860a18f0cf81e524078b0116400430",
	1501826872, 24, 13835058055284619847, chain2_block3_txs);
const auto chain2_block4_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, 0)
};
const auto chain2_block4 = std::make_shared<Block>(
	0, "0000006249ba6098b7ea1c6b1ab931d36a306a8bda8f3e632b703d84dda8b4b6",
	"8b0485e1a7823fc2ad3195837146e2b01e860a18f0cf81e524078b0116400430",
	1501826949, 24, 4611686018428394083, chain2_block4_txs);
const auto chain2_block5_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(), std::vector<uint8_t>(), -1)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, 0)
};
const auto chain2_block5 = std::make_shared<Block>(
	0, "0000002dca22d47151d6cb1a2e60c1af535174c0b2b0d4152c63b76d034edc6d",
	"8b0485e1a7823fc2ad3195837146e2b01e860a18f0cf81e524078b0116400430",
	1501827000, 24, 13835058055287732375, chain2_block5_txs);
const auto chain2 = std::vector{
	chain1_block1, chain2_block2, chain2_block3, chain2_block4, chain2_block5
};

TEST(BlockChainTest, Reorg)
{
	Chain::ActiveChain.clear();
	Chain::SideBranches.clear();
	Mempool::Map.clear();
	UTXO::Map.clear();

	for (const auto& block : chain1)
		ASSERT_EQ(Chain::ActiveChainIdx, Chain::ConnectBlock(block));

	Chain::SideBranches.clear();
	Mempool::Map.clear();
	UTXO::Map.clear();

	for (const auto& block : Chain::ActiveChain)
	{
		for (const auto& tx : block->Txs)
		{
			for (uint32_t i = 0; i < tx->TxOuts.size(); i++)
			{
				UTXO::AddToMap(tx->TxOuts[i], tx->Id(), i, tx->IsCoinbase(), Chain::ActiveChain.size());
			}
		}
	}

	ASSERT_EQ(3, UTXO::Map.size());
	ASSERT_FALSE(Chain::ReorgIfNecessary());

	ASSERT_EQ(1, Chain::ConnectBlock(chain2[1]));

	ASSERT_FALSE(Chain::ReorgIfNecessary());
	ASSERT_TRUE(Chain::SideBranches.size() == 1);
	ASSERT_EQ(*chain2[1], *Chain::SideBranches[0][0]);
	ASSERT_EQ(chain1.size(), Chain::ActiveChain.size());
	for (uint32_t i = 0; i < Chain::ActiveChain.size(); i++)
	{
		ASSERT_EQ(*chain1[i], *Chain::ActiveChain[i]);
	}
	ASSERT_TRUE(Mempool::Map.empty());
	const std::array<std::string, 3> tx_ids{ "b6678c", "b90f9b", "b6678c" };
	ASSERT_EQ(tx_ids.size(), UTXO::Map.size());
	for (const auto& k : UTXO::Map | std::views::keys)
	{
		bool found = false;
		for (const auto& tx_id : tx_ids)
		{
			found |= k->TxId.ends_with(tx_id);
			if (found)
			{
				break;
			}
		}
		ASSERT_TRUE(found);
	}

	ASSERT_EQ(1, Chain::ConnectBlock(chain2[2]));

	ASSERT_FALSE(Chain::ReorgIfNecessary());
	ASSERT_EQ(1, Chain::SideBranches.size());
	std::array side_branch_test{ chain2[1], chain2[2] };
	for (uint32_t i = 0; i < Chain::SideBranches[0].size(); i++)
	{
		ASSERT_EQ(*side_branch_test[i], *Chain::SideBranches[0][i]);
	}
	ASSERT_EQ(Chain::ActiveChain.size(), chain1.size());
	for (uint32_t i = 0; i < Chain::ActiveChain.size(); i++)
	{
		ASSERT_EQ(*chain1[i], *Chain::ActiveChain[i]);
	}
	ASSERT_TRUE(Mempool::Map.empty());
	ASSERT_EQ(tx_ids.size(), UTXO::Map.size());
	for (const auto& k : UTXO::Map | std::views::keys)
	{
		bool found = false;
		for (const auto& tx_id : tx_ids)
		{
			found |= k->TxId.ends_with(tx_id);
			if (found)
			{
				break;
			}
		}
		ASSERT_TRUE(found);
	}

	auto chain3_faulty = chain2;
	auto chain2_block4_copy = std::make_shared<Block>(*chain2_block4);
	chain2_block4_copy->Nonce = 1;
	chain3_faulty[3] = chain2_block4_copy;

	ASSERT_EQ(-1, Chain::ConnectBlock(chain3_faulty[3]));
	ASSERT_FALSE(Chain::ReorgIfNecessary());

	ASSERT_EQ(1, Chain::SideBranches.size());
	for (uint32_t i = 0; i < Chain::SideBranches[0].size(); i++)
	{
		ASSERT_EQ(*side_branch_test[i], *Chain::SideBranches[0][i]);
	}
	ASSERT_EQ(Chain::ActiveChain.size(), chain1.size());
	for (uint32_t i = 0; i < Chain::ActiveChain.size(); i++)
	{
		ASSERT_EQ(*chain1[i], *Chain::ActiveChain[i]);
	}
	ASSERT_TRUE(Mempool::Map.empty());
	ASSERT_EQ(tx_ids.size(), UTXO::Map.size());
	for (const auto& k : UTXO::Map | std::views::keys)
	{
		bool found = false;
		for (const auto& tx_id : tx_ids)
		{
			found |= k->TxId.ends_with(tx_id);
			if (found)
			{
				break;
			}
		}
		ASSERT_TRUE(found);
	}

	ASSERT_EQ(1, Chain::ConnectBlock(chain2[3]));
	ASSERT_EQ(1, Chain::ConnectBlock(chain2[4]));

	ASSERT_EQ(1, Chain::SideBranches.size());
	ASSERT_EQ(2, Chain::SideBranches[0].size());
	std::vector<std::string> side_branch_ids;
	side_branch_ids.reserve(Chain::SideBranches[0].size());
	for (const auto& block : Chain::SideBranches[0])
	{
		side_branch_ids.emplace_back(block->Id());
	}
	std::vector<std::string> chain1_ids;
	chain1_ids.reserve(chain1.size());
	for (uint32_t i = 1; i < chain1.size(); i++)
	{
		chain1_ids.emplace_back(chain1[i]->Id());
	}
	ASSERT_EQ(chain1_ids, side_branch_ids);
	std::array side_branch_test2{ chain1[1], chain1[2] };
	for (uint32_t i = 0; i < Chain::SideBranches[0].size(); i++)
	{
		ASSERT_EQ(*side_branch_test2[i], *Chain::SideBranches[0][i]);
	}
	ASSERT_TRUE(Mempool::Map.empty());
	const std::array<std::string, 5> tx_ids2{ "b90f9b", "b6678c", "b6678c", "b6678c", "b6678c" };
	ASSERT_EQ(UTXO::Map.size(), tx_ids2.size());
	for (const auto& k : UTXO::Map | std::views::keys)
	{
		bool found = false;
		for (const auto& tx_id : tx_ids2)
		{
			found |= k->TxId.ends_with(tx_id);
			if (found)
			{
				break;
			}
		}
		ASSERT_TRUE(found);
	}
}

#ifndef _DEBUG
TEST(BlockChainTest_LongRunning, DependentTxsInSingleBlock)
{
	Chain::ActiveChain.clear();
	Chain::SideBranches.clear();
	Mempool::Map.clear();
	UTXO::Map.clear();

	ASSERT_EQ(Chain::ActiveChainIdx, Chain::ConnectBlock(chain1[0]));
	ASSERT_EQ(Chain::ActiveChainIdx, Chain::ConnectBlock(chain1[1]));

	ASSERT_EQ(2, Chain::ActiveChain.size());
	ASSERT_EQ(2, UTXO::Map.size());

	auto priv_key = Utils::HexStringToByteArray("18e14a7b6a307f426a94f8114701e7c8e774e7f9a47e2c2035db29a206321725");
	auto pub_key = ECDSA::GetPubKeyFromPrivKey(priv_key);
	auto address = Wallet::PubKeyToAddress(pub_key);

	const auto& utxo1 = UTXO::Map.begin()->second;
	auto tx_out1 = std::make_shared<TxOut>(901, utxo1->TxOut->ToAddress);
	std::vector tx_outs1{ tx_out1 };
	auto tx_in1 = Wallet::BuildTxIn(priv_key, utxo1->TxOutPoint, tx_outs1);
	auto tx1 = std::make_shared<Tx>(std::vector{ tx_in1 }, tx_outs1, 0);

	ASSERT_THROW(
		{
		try
		{
		tx1->Validate(Tx::ValidateRequest());
		}
		catch (const TxValidationException& ex)
		{
		ASSERT_STREQ("Coinbase UTXO not ready for spending", ex.what());
		throw;
		}
		},
		TxValidationException);

	Chain::ConnectBlock(chain1[2]);

	Mempool::AddTxToMempool(tx1);
	ASSERT_TRUE(Mempool::Map.contains(tx1->Id()));

	auto tx_out2 = std::make_shared<TxOut>(9001, tx_out1->ToAddress);
	std::vector tx_outs2{ tx_out2 };
	auto tx_out_point2 = std::make_shared<TxOutPoint>(tx1->Id(), 0);
	auto tx_in2 = Wallet::BuildTxIn(priv_key, tx_out_point2, tx_outs2);
	auto tx2 = std::make_shared<Tx>(std::vector{ tx_in2 }, tx_outs2, 0);

	Mempool::AddTxToMempool(tx2);
	ASSERT_FALSE(Mempool::Map.contains(tx2->Id()));

	ASSERT_THROW(
		{
		try
		{
		tx2->Validate(Tx::ValidateRequest());
		}
		catch (const TxValidationException& ex)
		{
		ASSERT_STREQ("Spent value more than available", ex.what());
		throw;
		}
		},
		TxValidationException);

	tx_out2->Value = 901;
	tx_in2 = Wallet::BuildTxIn(priv_key, tx_out_point2, tx_outs2);
	tx2->TxIns[0] = tx_in2;

	Mempool::AddTxToMempool(tx2);
	ASSERT_TRUE(Mempool::Map.contains(tx2->Id()));

	auto block = PoW::AssembleAndSolveBlock(address);

	ASSERT_EQ(Chain::ActiveChainIdx, Chain::ConnectBlock(block));

	ASSERT_EQ(*Chain::ActiveChain.back(), *block);
	ASSERT_EQ(2, block->Txs.size() - 1);
	std::array txs{ tx1, tx2 };
	for (uint32_t i = 0; i < txs.size(); i++)
	{
		ASSERT_EQ(*txs[i], *block->Txs[i + 1]);
	}
	ASSERT_FALSE(Mempool::Map.contains(tx1->Id()));
	ASSERT_FALSE(Mempool::Map.contains(tx2->Id()));
	auto map_it1 = std::ranges::find_if(UTXO::Map,
	                                    [&tx1](const std::pair<std::shared_ptr<TxOutPoint>, std::shared_ptr<UTXO>>& p)
	                                    {
		                                    const auto& [txOutPoint, utxo] = p;
		                                    return txOutPoint->TxId == tx1->Id() && txOutPoint->TxOutIdx == 0;
	                                    });
	ASSERT_EQ(map_it1, UTXO::Map.end());
	auto map_it2 = std::ranges::find_if(UTXO::Map,
	                                    [&tx2](const std::pair<std::shared_ptr<TxOutPoint>, std::shared_ptr<UTXO>>& p)
	                                    {
		                                    const auto& [txOutPoint, utxo] = p;
		                                    return txOutPoint->TxId == tx2->Id() && txOutPoint->TxOutIdx == 0;
	                                    });
	ASSERT_NE(map_it2, UTXO::Map.end());
}

TEST(BlockChainTest_LongRunning, MinerTransaction)
{
	Chain::ActiveChain.clear();
	Chain::SideBranches.clear();
	Mempool::Map.clear();
	UTXO::Map.clear();

	const auto [miner_priv_key, miner_pub_key, miner_address] = Wallet::InitWallet("miner.dat");
	const auto [receiver_priv_key, receiver_pub_key, receiver_address] = Wallet::InitWallet("receiver.dat");

	const auto first_block = PoW::AssembleAndSolveBlock(miner_address);
	if (first_block == nullptr)
		FAIL();
	Chain::ConnectBlock(first_block);
	Chain::SaveToDisk();

	for (int i = 0; i < NetParams::COINBASE_MATURITY + 1; i++)
	{
		const auto maturity_blocks = PoW::AssembleAndSolveBlock(miner_address);
		if (maturity_blocks == nullptr)
			FAIL();
		Chain::ConnectBlock(maturity_blocks);
		Chain::SaveToDisk();
	}

	ASSERT_GT(Wallet::GetBalance_Miner(miner_address), 0);
	auto tx = Wallet::SendValue_Miner(first_block->Txs.front()->TxOuts.front()->Value / 2, 100, receiver_address,
	                                  miner_priv_key);
	ASSERT_TRUE(tx != nullptr);
	ASSERT_EQ(Wallet::GetTxStatus_Miner(tx->Id()).Status, TxStatus::Mempool);

	const auto post_tx_block = PoW::AssembleAndSolveBlock(miner_address);
	if (post_tx_block == nullptr)
		FAIL();
	Chain::ConnectBlock(post_tx_block);
	Chain::SaveToDisk();

	auto mined_tx_status = Wallet::GetTxStatus_Miner(tx->Id());
	ASSERT_EQ(mined_tx_status.Status, TxStatus::Mined);
	ASSERT_EQ(mined_tx_status.BlockId, post_tx_block->Id());
	ASSERT_GT(Wallet::GetBalance_Miner(receiver_address), 0);
}
#endif
