#include "pch.hpp"

#include <algorithm>
#include <fmt/format.h>

#include "Chain.hpp"
#include "Tx.hpp"
#include "TxIn.hpp"
#include "TxOut.hpp"
#include "Block.hpp"
#include "NetParams.hpp"
#include "Utils.hpp"
#include "Exceptions.hpp"
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

int64_t Chain::ActiveChainIdx;

int64_t Chain::GetCurrentHeight()
{
	std::lock_guard lock(Lock);

	return ActiveChain.size();
}

std::tuple<std::shared_ptr<Block>, int64_t, int64_t> Chain::LocateBlockInActiveChain(const std::string& blockHash)
{
	std::lock_guard lock(Lock);

	int64_t chain_idx = 0;
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

std::pair<std::shared_ptr<Block>, int64_t> Chain::LocateBlockInChain(const std::string& blockHash, const std::vector<std::shared_ptr<Block>>& chain)
{
	std::lock_guard lock(Lock);

	int64_t height = 0;
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

	return nullptr;
}

std::pair<std::shared_ptr<Block>, int64_t> Chain::ValidateBlock(const std::shared_ptr<Block>& block)
{
	std::lock_guard lock(Lock);

	const auto& txs = block->Txs;

	if (txs.empty())
		throw BlockValidationException("Transactions are empty");

	auto now = Utils::GetUnixTimestamp();
	if (block->Timestamp - now > NetParams::MAX_FUTURE_BLOCK_TIME_IN_SECS)
		throw BlockValidationException("Block timestamp is too far in future");

	if (std::stoul(block->Id(), nullptr, 16) > (1 << (256 - block->Bits)))
		throw BlockValidationException("Block header does not satisfy bits");

	auto coinbase_pred = [](const std::shared_ptr<Tx>& tx)
	{
		return tx->IsCoinbase();
	};
	auto coinbase_it = std::find_if(txs.begin(), txs.end(), coinbase_pred);
	if (coinbase_it != txs.begin() || std::find_if(++coinbase_it, txs.end(), coinbase_pred) != txs.end())
		throw BlockValidationException("First transaction must be coinbase and no more");

	for (size_t i = 0; i < txs.size(); i++)
	{
		try
		{
			txs[i]->ValidateBasics(i == 0);

		}
		catch (const TxValidationException&)
		{
			throw BlockValidationException(fmt::format("Transaction {} is invalid", txs[i]->Id()).c_str());
		}
	}

	std::vector<std::string> hashes;
	hashes.reserve(txs.size());
	for (const auto& tx : txs)
	{
		hashes.push_back(tx->Id());
	}
	auto merkleRoot = MerkleTree::GetRoot(hashes);
	if (merkleRoot->Value != block->MerkleHash)
		throw BlockValidationException("Merkle hash is invalid");

	if (block->Timestamp <= GetMedianTimePast(11))
		throw BlockValidationException("Timestamp is too old");

	int64_t prev_block_chain_idx = 0;
	if (block->PrevBlockHash.empty())
	{
		prev_block_chain_idx = ActiveChainIdx;
	}
	else
	{
		auto [prev_block, prev_block_height, prev_block_chain_idx] = LocateBlockInActiveChain(block->PrevBlockHash);
		if (prev_block == nullptr)
			throw BlockValidationException(fmt::format("Previous block {} is not found in any chain", block->PrevBlockHash).c_str(), block);

		if (prev_block_chain_idx != ActiveChainIdx)
			return { block, prev_block_chain_idx };
		else if (prev_block->Id() != ActiveChain.back()->Id())
			return { block, prev_block_chain_idx + 1 };
	}

	if (PoW::GetNextWorkRequired(block->PrevBlockHash) != block->Bits)
		throw BlockValidationException("Bits are incorrect");

	std::vector<std::shared_ptr<Tx>> nonCoinbaseTxs(block->Txs.begin() + 1, block->Txs.end());
	Tx::ValidateRequest req;
	req.SiblingsInBlock = nonCoinbaseTxs;
	req.Allow_UTXO_FromMempool = false;
	for (const auto& nonCoinbaseTx : nonCoinbaseTxs)
	{
		try
		{
			nonCoinbaseTx->Validate(req);
		}
		catch (const TxValidationException&)
		{
			throw BlockValidationException(fmt::format("Transaction {} failed to validate", nonCoinbaseTx->Id()).c_str());
		}
	}

	return { block, prev_block_chain_idx };
}

int64_t Chain::GetMedianTimePast(size_t numLastBlocks)
{
	if (numLastBlocks > ActiveChain.size())
		return 0;

	size_t first_idx = ActiveChain.size() - numLastBlocks;
	size_t median_idx = first_idx + (numLastBlocks / 2);

	return ActiveChain[median_idx]->Timestamp;
}
