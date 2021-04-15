#include "pch.hpp"

#include <exception>
#include <algorithm>

#include "Chain.hpp"
#include "NetParams.hpp"
#include "MerkleTree.hpp"
#include "PoW.hpp"

const std::shared_ptr<TxIn> Chain::GenesisTxIn = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{ 0x00 }, std::vector<uint8_t>(), 0);
const std::shared_ptr<TxOut> Chain::GenesisTxOut = std::make_shared<TxOut>(5000000000, "143UVyz7ooiAv1pMqbwPPpnH4BV9ifJGFF");
const std::shared_ptr<Tx> Chain::GenesisTx = std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ Chain::GenesisTxIn },
	std::vector<std::shared_ptr<TxOut>>{ Chain::GenesisTxOut }, 0);
const std::shared_ptr<Block> Chain::GenesisBlock = std::make_shared<Block>(0, "", "7118894203235a955a908c0abfc6d8fe6edec47b0a04ce1bf7263da3b4366d22",
	1501821412, 24, 10126761, std::vector<std::shared_ptr<Tx>>{ GenesisTx });

std::vector<std::shared_ptr<Block>> Chain::ActiveChain = std::vector<std::shared_ptr<Block>>{ GenesisBlock };
std::vector<std::vector<std::shared_ptr<Block>>> Chain::SideBranches = std::vector<std::vector<std::shared_ptr<Block>>>{ };
std::vector<std::shared_ptr<Block>> Chain::OrphanBlocks = std::vector<std::shared_ptr<Block>>{ };

std::recursive_mutex Chain::Lock;

int32_t Chain::ActiveChainIdx;

int32_t Chain::GetCurrentHeight()
{
	std::lock_guard lock(Lock);

	return ActiveChain.size();
}

std::tuple<std::shared_ptr<Block>, int32_t, int32_t> Chain::LocateBlockInActiveChain(const std::string& blockHash)
{
	std::lock_guard lock(Lock);

	int32_t chain_idx = 0;
	auto [located_block, located_block_height] = LocateBlockInChain(blockHash, ActiveChain);
	if (located_block != nullptr)
		return { located_block, located_block_height, chain_idx };

	for (const auto& side_chain : SideBranches)
	{
		chain_idx++;

		auto [located_block, located_block_height] = LocateBlockInChain(blockHash, side_chain);
		if (located_block != nullptr)
			return { located_block, located_block_height, chain_idx };
	}

	return { nullptr, -1, -1 };
}

std::pair<std::shared_ptr<Block>, int32_t> Chain::LocateBlockInChain(const std::string& blockHash, const std::vector<std::shared_ptr<Block>>& chain)
{
	std::lock_guard lock(Lock);

	int32_t height = 0;
	for (const auto& block : chain)
	{
		if (block->Id() == blockHash)
		{
			return { block, height };
		}

		height++;
	}

	return { nullptr, -1 };
}

std::shared_ptr<Block> Chain::ConnectBlock(const std::shared_ptr<Block>& block, bool doingReorg /*= false*/)
{
	std::lock_guard lock(Lock);

	if (doingReorg)
	{
		auto [located_block, located_block_height, located_block_chain_idx] = LocateBlockInActiveChain(block->Id());
		if (located_block != nullptr)
		{
			return nullptr;
		}
	}

	//TODO: validate block
}

std::pair<std::shared_ptr<Block>, int32_t> Chain::ValidateBlock(const std::shared_ptr<Block>& block)
{
	std::lock_guard lock(Lock);

	if (block->Txs.empty())
		throw std::exception("Chain::ValidateBlock --- block->Txs.empty()");

	auto now = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	if (block->Timestamp - now > NetParams::MAX_FUTURE_BLOCK_TIME_IN_SECS)
		throw std::exception("Chain::ValidateBlock --- block->Timestamp - now > NetParams::MAX_FUTURE_BLOCK_TIME_IN_SECS");

	if (std::stoul(block->Id(), nullptr, 16) > (1 << (256 - block->Bits)))
		throw std::exception("Chain::ValidateBlock --- std::stoul(block->Id(), nullptr, 16) > (1 << (256 - block->Bits)");

	auto coinbase_pred = [](const std::shared_ptr<Tx>& tx)
	{
		return tx->IsCoinbase();
	};
	auto coinbase_it = std::find_if(block->Txs.begin(), block->Txs.end(), coinbase_pred);
	if (coinbase_it != block->Txs.begin() || std::find_if(++coinbase_it, block->Txs.end(), coinbase_pred) != block->Txs.end())
		throw std::exception("Chain::ValidateBlock --- coinbase_it != block->Txs.begin() || std::find_if(++coinbase_it, block->Txs.end(), coinbase_pred) != block->Txs.end()");

	try
	{
		for (int i = 0; i < block->Txs.size(); i++)
		{
			block->Txs[i]->ValidateBasics(i == 0);
		}
	}
	catch (...)
	{
		throw std::exception("Chain::ValidateBlock --- block->Txs[i]->Validate(i == 0)");
	}

	std::vector<std::string> hashes;
	hashes.reserve(block->Txs.size());
	for (const auto& tx : block->Txs)
	{
		hashes.push_back(tx->Id());
	}
	auto merkleRoot = MerkleTree::GetRoot(hashes);
	if (merkleRoot->Value != block->MerkleHash)
		throw std::exception("Chain::ValidateBlock --- merkleRoot->Value != block->MerkleHash");

	if (block->Timestamp <= GetMedianTimePast(11))
		throw std::exception("Chain::ValidateBlock --- block->Timestamp <= GetMedianTimePast(11)");

	int32_t prev_block_chain_idx = 0;
	if (block->PrevBlockHash.empty())
	{
		prev_block_chain_idx = ActiveChainIdx;
	}
	else
	{
		auto [prev_block, prev_block_height, prev_block_chain_idx] = LocateBlockInActiveChain(block->PrevBlockHash);
		if (prev_block == nullptr)
			throw std::exception("Chain::ValidateBlock --- prev_block == nullptr");

		if (prev_block_chain_idx != ActiveChainIdx)
			return { block, prev_block_chain_idx };
		else if (prev_block->Id() != ActiveChain.back()->Id())
			return { block, prev_block_chain_idx + 1 };
	}

	if (PoW::GetNextWorkRequired(block->PrevBlockHash) != block->Bits)
		throw std::exception("Chain::ValidateBlock --- PoW::GetNextWorkRequired(block->PrevBlockHash) != block->Bits");

	//TODO: validate tx
}

int64_t Chain::GetMedianTimePast(size_t numLastBlocks)
{
	if (numLastBlocks > ActiveChain.size())
		return 0;

	size_t first_idx = ActiveChain.size() - numLastBlocks;
	size_t median_idx = first_idx + (numLastBlocks / 2);

	return ActiveChain[median_idx]->Timestamp;
}
