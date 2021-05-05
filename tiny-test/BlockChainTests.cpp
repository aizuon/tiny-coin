#include "pch.hpp"

#include <cstdint>
#include <array>
#include <vector>
#include <memory>
#include <algorithm>

#include "gtest/gtest.h"

#include "../tiny-lib/Exceptions.hpp"
#include "../tiny-lib/Utils.hpp"
#include "../tiny-lib/Block.hpp"
#include "../tiny-lib/Chain.hpp"
#include "../tiny-lib/Mempool.hpp"
#include "../tiny-lib/PoW.hpp"
#include "../tiny-lib/ECDSA.hpp"
#include "../tiny-lib/Wallet.hpp"
#include "../tiny-lib/Tx.hpp"
#include "../tiny-lib/TxOut.hpp"
#include "../tiny-lib/UnspentTxOut.hpp"

TEST(BlockChainTest, MedianTimePast)
{
	Chain::ActiveChain = std::vector<std::shared_ptr<Block>>();

	EXPECT_TRUE(Chain::GetMedianTimePast(10) == 0);

	std::array<int64_t, 5> timestamps{1, 30, 60, 90, 400};

	for (auto timestamp : timestamps)
	{
		auto dummyBlock = std::make_shared<Block>(0, "foo", "foo", timestamp, 1, 0, std::vector<std::shared_ptr<Tx>>());

		Chain::ActiveChain.push_back(dummyBlock);
	}

	EXPECT_EQ(Chain::GetMedianTimePast(1), 400);
	EXPECT_EQ(Chain::GetMedianTimePast(3), 90);
	EXPECT_EQ(Chain::GetMedianTimePast(2), 90);
	EXPECT_EQ(Chain::GetMedianTimePast(5), 60);
}

auto chain1_block1_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{0x00}, std::vector<uint8_t>(), 0)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "143UVyz7ooiAv1pMqbwPPpnH4BV9ifJGFF")
		}, -1)
};
auto chain1_block1 = std::make_shared<Block>(0, "", "8fdcb01b725d0dba8437ab9fd20714acc5b6ff0ea7a3a052d72318ab234b5d0d",
                                             1501821412, 24, 60590211, chain1_block1_txs);
auto chain1_block2_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{0x01}, std::vector<uint8_t>(), 0)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, -1)
};
auto chain1_block2 = std::make_shared<Block>(0, "0000009aefdd6ab38b5549dec09039ab64f5055892cb49f309568ffd9fedb629",
                                             "4eced71470b00a09aa7d61e6acb12a0aad2864593504072c8acf39f87da48cec",
                                             1501826444, 24, 12171145, chain1_block2_txs);
auto chain1_block3_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{0x02}, std::vector<uint8_t>(), 0)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, -1)
};
auto chain1_block3 = std::make_shared<Block>(0, "0000005d284aa9f5448f1f74f18d36204cdb9c5f673a5f1b35340a0f83fdb23c",
                                             "e8c33875847f0eb422e3de129e258a7f0c90b65fa28e5d0dbf6b13b41f4e5415",
                                             1501826556, 24, 53695323, chain1_block3_txs);
auto chain1 = std::vector{chain1_block1, chain1_block2, chain1_block3};

auto chain2_block1_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{0x00}, std::vector<uint8_t>(), 0)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "143UVyz7ooiAv1pMqbwPPpnH4BV9ifJGFF")
		}, -1)
};
auto chain2_block1 = std::make_shared<Block>(0, "", "8fdcb01b725d0dba8437ab9fd20714acc5b6ff0ea7a3a052d72318ab234b5d0d",
                                             1501821412, 24, 60590211, chain2_block1_txs);
auto chain2_block2_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{0x01}, std::vector<uint8_t>(), 0)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, -1)
};
auto chain2_block2 = std::make_shared<Block>(0, "0000009aefdd6ab38b5549dec09039ab64f5055892cb49f309568ffd9fedb629",
                                             "4eced71470b00a09aa7d61e6acb12a0aad2864593504072c8acf39f87da48cec",
                                             1501826757, 24, 18122613, chain2_block2_txs);
auto chain2_block3_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{0x02}, std::vector<uint8_t>(), 0)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, -1)
};
auto chain2_block3 = std::make_shared<Block>(0, "000000eb4d21e850a00cb509394d944e402ff723accfc8c0b81e6fbe88f0d53a",
                                             "e8c33875847f0eb422e3de129e258a7f0c90b65fa28e5d0dbf6b13b41f4e5415",
                                             1501826872, 24, 3758387, chain2_block3_txs);
auto chain2_block4_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{0x03}, std::vector<uint8_t>(), 0)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, -1)
};
auto chain2_block4 = std::make_shared<Block>(0, "000000c611569bda6c484145f297c89c4774dfce133a26b5345c8098685dce1b",
                                             "e4ddedd960dcd9611f312a9aad74c3364ed02f2244cfb2e90ffc1effab47fc0b",
                                             1501826949, 24, 31463847, chain2_block4_txs);
auto chain2_block5_txs = std::vector{
	std::make_shared<Tx>(
		std::vector{
			std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{0x04}, std::vector<uint8_t>(), 0)
		}, std::vector{
			std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA")
		}, -1)
};
auto chain2_block5 = std::make_shared<Block>(0, "000000a3139dfb06800b631d2fdb2b0b445ed5d8538b85a1ef453482fbd9ffed",
                                             "89e6f44107581323b918c25dc9917c1baf87e8a2de884d7de093b1b083514583",
                                             1501827000, 24, 25006008, chain2_block5_txs);
auto chain2 = std::vector{
	chain2_block1, chain2_block2, chain2_block3, chain2_block4, chain2_block5
};

TEST(BlockChainTest, DependentTxsInSingleBlock)
{
	Chain::ActiveChain = std::vector<std::shared_ptr<Block>>();
	Mempool::Map = std::unordered_map<std::string, std::shared_ptr<Tx>>();

	ASSERT_EQ(Chain::ConnectBlock(chain1[0]), Chain::ActiveChainIdx);
	ASSERT_EQ(Chain::ConnectBlock(chain1[1]), Chain::ActiveChainIdx);

	ASSERT_TRUE(Chain::ActiveChain.size() == 2);
	ASSERT_TRUE(UTXO::Map.size() == 2);

	const auto& utxo1 = UTXO::Map.begin()->second;
	auto txOut1 = std::make_shared<TxOut>(901, utxo1->TxOut->ToAddress);
	auto priv_key = Utils::HexStringToByteArray("f1ad3279bfa278ab6efb4f98f7a7b4c0f4664f7a58bff65cd2cb2d1d3a3020a7");
	auto pub_key = ECDSA::GetPubKeyFromPrivKey(priv_key);
	auto address = Wallet::PubKeyToAddress(pub_key);
	auto txIn1 = Wallet::MakeTxIn(priv_key, utxo1->TxOutPoint, txOut1);
	auto tx1 = std::make_shared<Tx>(std::vector{txIn1},
	                                std::vector{txOut1}, 0);

	auto txOut2 = std::make_shared<TxOut>(9001, txOut1->ToAddress);
	auto txOutPoint2 = std::make_shared<TxOutPoint>(tx1->Id(), 0);
	auto txIn2 = Wallet::MakeTxIn(priv_key, txOutPoint2, txOut2);
	auto tx2 = std::make_shared<Tx>(std::vector{txIn2},
	                                std::vector{txOut2}, 0);

	ASSERT_THROW({
	             try
	             {
	             tx2->Validate(Tx::ValidateRequest());
	             }
	             catch (const TxValidationException& ex)
	             {
	             ASSERT_STREQ("Coinbase UTXO is not ready for spending", ex.what());
	             throw;
	             }
	             }, TxValidationException);

	Chain::ConnectBlock(chain1[2]);

	Mempool::AddTxToMempool(tx1);
	ASSERT_TRUE(Mempool::Map.contains(tx1->Id()));

	Mempool::AddTxToMempool(tx2);
	ASSERT_FALSE(Mempool::Map.contains(tx2->Id()));

	ASSERT_THROW({
	             try
	             {
	             tx2->Validate(Tx::ValidateRequest());
	             }
	             catch (const TxValidationException& ex)
	             {
	             ASSERT_STREQ("Spend value is more than available", ex.what());
	             throw;
	             }
	             }, TxValidationException);

	txOut2 = std::make_shared<TxOut>(901, txOut1->ToAddress);
	txOutPoint2 = std::make_shared<TxOutPoint>(tx1->Id(), 0);
	txIn2 = Wallet::MakeTxIn(priv_key, txOutPoint2, txOut2);
	tx2 = std::make_shared<Tx>(std::vector{txIn2}, std::vector{txOut2},
	                           0);

	Mempool::AddTxToMempool(tx2);
	ASSERT_TRUE(Mempool::Map.contains(tx2->Id()));

	auto block = PoW::AssembleAndSolveBlock(address);

	ASSERT_EQ(Chain::ConnectBlock(block), Chain::ActiveChainIdx);

	ASSERT_EQ(*Chain::ActiveChain.back(), *block);
	ASSERT_EQ(block->Txs.size() - 1, 2);
	std::array txs{tx1, tx2};
	for (size_t i = 0; i < txs.size(); i++)
	{
		ASSERT_EQ(*block->Txs[i + 1], *txs[i]);
	}
	ASSERT_FALSE(Mempool::Map.contains(tx1->Id()));
	ASSERT_FALSE(Mempool::Map.contains(tx2->Id()));
	auto map_it1 = std::find_if(UTXO::Map.begin(), UTXO::Map.end(),
	                            [&tx1](const std::pair<std::shared_ptr<TxOutPoint>, std::shared_ptr<UnspentTxOut>>& p)
	                            {
		                            const auto& [txOutPoint, utxo] = p;
		                            return txOutPoint->TxId == tx1->Id() && txOutPoint->TxOutIdx == 0;
	                            });
	ASSERT_EQ(map_it1, UTXO::Map.end());
	auto map_it2 = std::find_if(UTXO::Map.begin(), UTXO::Map.end(),
	                            [&tx2](const std::pair<std::shared_ptr<TxOutPoint>, std::shared_ptr<UnspentTxOut>>& p)
	                            {
		                            const auto& [txOutPoint, utxo] = p;
		                            return txOutPoint->TxId == tx2->Id() && txOutPoint->TxOutIdx == 0;
	                            });
	ASSERT_NE(map_it2, UTXO::Map.end());
}

TEST(BlockChainTest, Reorg)
{
	Chain::ActiveChain = std::vector<std::shared_ptr<Block>>();

	for (const auto& block : chain1)
		ASSERT_EQ(Chain::ConnectBlock(block), Chain::ActiveChainIdx);

	Chain::SideBranches = std::vector<std::vector<std::shared_ptr<Block>>>();
	Mempool::Map = std::unordered_map<std::string, std::shared_ptr<Tx>>();
	UTXO::Map = std::unordered_map<std::shared_ptr<TxOutPoint>, std::shared_ptr<UnspentTxOut>>();

	for (const auto& block : Chain::ActiveChain)
	{
		for (const auto& tx : block->Txs)
		{
			for (int64_t i = 0; i < tx->TxOuts.size(); i++)
			{
				UTXO::AddToMap(tx->TxOuts[i], tx->Id(), i, tx->IsCoinbase(), Chain::ActiveChain.size());
			}
		}
	}

	ASSERT_TRUE(UTXO::Map.size() == 3);
	ASSERT_FALSE(Chain::ReorgIfNecessary());

	ASSERT_TRUE(Chain::ConnectBlock(chain2[1]) == 1);

	ASSERT_FALSE(Chain::ReorgIfNecessary());
	ASSERT_TRUE(Chain::SideBranches.size() == 1);
	ASSERT_EQ(*Chain::SideBranches[0][0], *chain2[1]);
	ASSERT_EQ(Chain::ActiveChain.size(), chain1.size());
	for (size_t i = 0; i < Chain::ActiveChain.size(); i++)
	{
		ASSERT_EQ(*Chain::ActiveChain[i], *chain1[i]);
	}
	ASSERT_TRUE(Mempool::Map.empty());
	const std::array<std::string, 3> txIds{"eab1ea", "7bbb46", "c3c8ab"};
	ASSERT_EQ(UTXO::Map.size(), txIds.size());
	for (const auto& [k, v] : UTXO::Map)
	{
		bool found = false;
		for (const auto& txId : txIds)
		{
			found |= k->TxId.ends_with(txId);
			if (found)
			{
				break;
			}
		}
		ASSERT_TRUE(found);
	}

	ASSERT_TRUE(Chain::ConnectBlock(chain2[2]) == 1);

	ASSERT_FALSE(Chain::ReorgIfNecessary());
	ASSERT_TRUE(Chain::SideBranches.size() == 1);
	std::array sideBranchTest{chain2[1], chain2[2]};
	for (size_t i = 0; i < Chain::SideBranches[0].size(); i++)
	{
		ASSERT_EQ(*Chain::SideBranches[0][i], *sideBranchTest[i]);
	}
	ASSERT_EQ(Chain::ActiveChain.size(), chain1.size());
	for (size_t i = 0; i < Chain::ActiveChain.size(); i++)
	{
		ASSERT_EQ(*Chain::ActiveChain[i], *chain1[i]);
	}
	ASSERT_TRUE(Mempool::Map.empty());
	ASSERT_EQ(UTXO::Map.size(), txIds.size());
	for (const auto& [k, v] : UTXO::Map)
	{
		bool found = false;
		for (const auto& txId : txIds)
		{
			found |= k->TxId.ends_with(txId);
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

	ASSERT_TRUE(Chain::ConnectBlock(chain3_faulty[3]) == -1);
	ASSERT_FALSE(Chain::ReorgIfNecessary());

	ASSERT_TRUE(Chain::SideBranches.size() == 1);
	for (size_t i = 0; i < Chain::SideBranches[0].size(); i++)
	{
		ASSERT_EQ(*Chain::SideBranches[0][i], *sideBranchTest[i]);
	}
	ASSERT_EQ(Chain::ActiveChain.size(), chain1.size());
	for (size_t i = 0; i < Chain::ActiveChain.size(); i++)
	{
		ASSERT_EQ(*Chain::ActiveChain[i], *chain1[i]);
	}
	ASSERT_TRUE(Mempool::Map.empty());
	ASSERT_EQ(UTXO::Map.size(), txIds.size());
	for (const auto& [k, v] : UTXO::Map)
	{
		bool found = false;
		for (const auto& txId : txIds)
		{
			found |= k->TxId.ends_with(txId);
			if (found)
			{
				break;
			}
		}
		ASSERT_TRUE(found);
	}

	ASSERT_TRUE(Chain::ConnectBlock(chain2[3]) == 1);
	ASSERT_TRUE(Chain::ConnectBlock(chain2[4]) == 1);

	ASSERT_TRUE(Chain::SideBranches.size() == 1);
	ASSERT_TRUE(Chain::SideBranches[0].size() == 2);
	std::vector<std::string> sideBranchIds;
	sideBranchIds.reserve(Chain::SideBranches[0].size());
	for (const auto& block : Chain::SideBranches[0])
	{
		sideBranchIds.emplace_back(block->Id());
	}
	std::vector<std::string> chain1Ids;
	chain1Ids.reserve(chain1.size());
	for (size_t i = 1; i < chain1.size(); i++)
	{
		chain1Ids.emplace_back(chain1[i]->Id());
	}
	ASSERT_EQ(sideBranchIds, chain1Ids);
	std::array sideBranchTest2{chain1[1], chain1[2]};
	for (size_t i = 0; i < Chain::SideBranches[0].size(); i++)
	{
		ASSERT_EQ(*Chain::SideBranches[0][i], *sideBranchTest2[i]);
	}
	ASSERT_TRUE(Mempool::Map.empty());
	const std::array<std::string, 5> txIds2{"eab1ea", "7bbb46", "c3c8ab", "f52068", "b3df02"};
	ASSERT_EQ(UTXO::Map.size(), txIds2.size());
	for (const auto& [k, v] : UTXO::Map)
	{
		bool found = false;
		for (const auto& txId : txIds2)
		{
			found |= k->TxId.ends_with(txId);
			if (found)
			{
				break;
			}
		}
		ASSERT_TRUE(found);
	}
}
