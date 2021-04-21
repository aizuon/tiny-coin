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

	std::array<int64_t, 5> timestamps{ 1, 30, 60, 90, 400 };

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

auto chain1_block1_txs = std::vector<std::shared_ptr<Tx>>{ std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{ 0x00 }, std::vector<uint8_t>(), 0) }, std::vector<std::shared_ptr<TxOut>>{ std::make_shared<TxOut>(5000000000, "143UVyz7ooiAv1pMqbwPPpnH4BV9ifJGFF") }, -1) };
auto chain1_block1 = std::make_shared<Block>(0, "", "d445c16c304784f3d89c2f65a53bb9f9d0b5e27a87dc1a33770664b62b424a3c", 1501821412, 24, 49160650, chain1_block1_txs);
auto chain1_block2_txs = std::vector<std::shared_ptr<Tx>>{ std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{ 0x01 }, std::vector<uint8_t>(), 0) }, std::vector<std::shared_ptr<TxOut>>{ std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA") }, -1) };
auto chain1_block2 = std::make_shared<Block>(0, "000000f3160fe69119aa6d1387964d0136bb8e7f52e5cf0ab8e2e769969ea541", "63818f170eb126c9c827c9183e9f611c6cb39cd18bab82f8421fe87c13b44304", 1501826444, 24, 40740373, chain1_block2_txs);
auto chain1_block3_txs = std::vector<std::shared_ptr<Tx>>{ std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{ 0x02 }, std::vector<uint8_t>(), 0) }, std::vector<std::shared_ptr<TxOut>>{ std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA") }, -1) };
auto chain1_block3 = std::make_shared<Block>(0, "000000a3c54df8964dfe3713a64e2b570b75bb8c9f3c1c39abb056b837c805df", "fa349cc448c31ed701f18eeb9bb788fd646efab168f4caa9952e75840559b4eb", 1501826556, 24, 3101452, chain1_block3_txs);
auto chain1 = std::vector<std::shared_ptr<Block>>{ chain1_block1, chain1_block2, chain1_block3 };

auto chain2_block1_txs = std::vector<std::shared_ptr<Tx>>{ std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{ 0x00 }, std::vector<uint8_t>(), 0) }, std::vector<std::shared_ptr<TxOut>>{ std::make_shared<TxOut>(5000000000, "143UVyz7ooiAv1pMqbwPPpnH4BV9ifJGFF") }, -1) };
auto chain2_block1 = std::make_shared<Block>(0, "", "d445c16c304784f3d89c2f65a53bb9f9d0b5e27a87dc1a33770664b62b424a3c", 1501821412, 24, 49160650, chain2_block1_txs);
auto chain2_block2_txs = std::vector<std::shared_ptr<Tx>>{ std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{ 0x01 }, std::vector<uint8_t>(), 0) }, std::vector<std::shared_ptr<TxOut>>{ std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA") }, -1) };
auto chain2_block2 = std::make_shared<Block>(0, "000000f3160fe69119aa6d1387964d0136bb8e7f52e5cf0ab8e2e769969ea541", "63818f170eb126c9c827c9183e9f611c6cb39cd18bab82f8421fe87c13b44304", 1501826757, 24, 20137768, chain2_block2_txs);
auto chain2_block3_txs = std::vector<std::shared_ptr<Tx>>{ std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{ 0x02 }, std::vector<uint8_t>(), 0) }, std::vector<std::shared_ptr<TxOut>>{ std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA") }, -1) };
auto chain2_block3 = std::make_shared<Block>(0, "000000a786e4e510e222496c1f642d615198f41ba659736a7fa8f45a78d017bb", "fa349cc448c31ed701f18eeb9bb788fd646efab168f4caa9952e75840559b4eb", 1501826872, 24, 37034282, chain2_block3_txs);
auto chain2_block4_txs = std::vector<std::shared_ptr<Tx>>{ std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{ 0x03 }, std::vector<uint8_t>(), 0) }, std::vector<std::shared_ptr<TxOut>>{ std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA") }, -1) };
auto chain2_block4 = std::make_shared<Block>(0, "000000d183865ef97c86d103daf63c0c50668bd2667618721e3392a090a32163", "34bba28730fcb681faa810c66a959aee010e1b837a2b4dea84477c983585219b", 1501826949, 24, 40171301, chain2_block4_txs);
auto chain2_block5_txs = std::vector<std::shared_ptr<Tx>>{ std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{ 0x04 }, std::vector<uint8_t>(), 0) }, std::vector<std::shared_ptr<TxOut>>{ std::make_shared<TxOut>(5000000000, "1Piq91dFUqSb7tdddCWvuGX5UgdzXeoAwA") }, -1) };
auto chain2_block5 = std::make_shared<Block>(0, "000000da45d61cab6e2ec33c36452418a2f188de9c3f37b4e04ba448cf6843e3", "8ac9e36a39dfc0b5242977470955e53545eee42f60bcf06e2e2891376516083a", 1501827000, 24, 19653316, chain2_block5_txs);
auto chain2 = std::vector<std::shared_ptr<Block>>{ chain2_block1, chain2_block2, chain2_block3, chain2_block4, chain2_block5 };

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
	auto tx1 = std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ txIn1 }, std::vector <std::shared_ptr<TxOut>>{ txOut1 }, 0);

	auto txOut2 = std::make_shared<TxOut>(9001, txOut1->ToAddress);
	auto txOutPoint2 = std::make_shared<TxOutPoint>(tx1->Id(), 0);
	auto txIn2 = Wallet::MakeTxIn(priv_key, txOutPoint2, txOut2);
	auto tx2 = std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ txIn2 }, std::vector <std::shared_ptr<TxOut>>{ txOut2 }, 0);

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
	tx2 = std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ txIn2 }, std::vector <std::shared_ptr<TxOut>>{ txOut2 }, 0);

	Mempool::AddTxToMempool(tx2);
	ASSERT_TRUE(Mempool::Map.contains(tx2->Id()));

	auto block = PoW::AssembleAndSolveBlock(address);

	ASSERT_EQ(Chain::ConnectBlock(block), Chain::ActiveChainIdx);

	ASSERT_EQ(*Chain::ActiveChain.back(), *block);
	ASSERT_EQ(block->Txs.size() - 1, 2);
	std::array<std::shared_ptr<Tx>, 2> txs{ tx1, tx2 };
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
	UTXO::Map = std::unordered_map<std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>>();

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
	const std::array<std::string, 3> txIds{ "9b42f6", "e58973", "0e738d" };
	ASSERT_EQ(UTXO::Map.size(), txIds.size());
	for (const auto& [k, v] : UTXO::Map)
	{
		bool found = false;
		for (const auto& txId : txIds)
		{
			found |= k->TxId.ends_with(txId);
		}
		ASSERT_TRUE(found);
	}

	ASSERT_TRUE(Chain::ConnectBlock(chain2[2]) == 1);

	ASSERT_FALSE(Chain::ReorgIfNecessary());
	ASSERT_TRUE(Chain::SideBranches.size() == 1);
	std::array<std::shared_ptr<Block>, 2> sideBranchTest{ chain2[1], chain2[2] };
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
	std::array<std::shared_ptr<Block>, 2> sideBranchTest2{ chain1[1], chain1[2] };
	for (size_t i = 0; i < Chain::SideBranches[0].size(); i++)
	{
		ASSERT_EQ(*Chain::SideBranches[0][i], *sideBranchTest2[i]);
	}
	ASSERT_TRUE(Mempool::Map.empty());
	const std::array<std::string, 5> txIds2{ "9b42f6", "0e738d", "e58973", "2bdc57", "7047d8" };
	ASSERT_EQ(UTXO::Map.size(), txIds2.size());
	for (const auto& [k, v] : UTXO::Map)
	{
		bool found = false;
		for (const auto& txId : txIds2)
		{
			found |= k->TxId.ends_with(txId);
		}
		ASSERT_TRUE(found);
	}
}