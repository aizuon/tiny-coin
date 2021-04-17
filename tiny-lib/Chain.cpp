#include "pch.hpp"

#include <cassert>
#include <exception>
#include <algorithm>
#include <fmt/format.h>

#include "Chain.hpp"
#include "Tx.hpp"
#include "TxIn.hpp"
#include "TxOut.hpp"
#include "TxOutPoint.hpp"
#include "UnspentTxOut.hpp"
#include "Block.hpp"
#include "Mempool.hpp"
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

int64_t Chain::GetCurrentHeight()
{
	std::lock_guard lock(Lock);

	return ActiveChain.size();
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

int64_t Chain::ConnectBlock(const std::shared_ptr<Block>& block, bool doingReorg /*= false*/)
{
	std::lock_guard lock(Lock);

	if (doingReorg)
	{
		auto [located_block, located_block_height, located_block_chain_idx] = LocateBlockInActiveChain(block->Id());
		if (located_block != nullptr)
		{
			return -1;
		}
	}

	int64_t chainIdx = -1;
	try
	{
		chainIdx = ValidateBlock(block);
	}
	catch (const BlockValidationException& ex)
	{
		if (ex.ToOrphan != nullptr)
		{
			OrphanBlocks.push_back(ex.ToOrphan);
		}

		return -1;
	}

	if (chainIdx == ActiveChainIdx && SideBranches.size() < chainIdx)
	{
		SideBranches.emplace_back();
	}

	auto chain = chainIdx == ActiveChainIdx ? ActiveChain : SideBranches[chainIdx - 1];
	chain.push_back(block);

	if (chainIdx == ActiveChainIdx)
	{
		for (const auto& tx : block->Txs)
		{
			Mempool::Map.erase(tx->Id());

			if (!tx->IsCoinbase())
			{
				for (const auto& txIn : tx->TxIns)
				{
					UnspentTxOut::RemoveFromMap(txIn->ToSpend->TxId, txIn->ToSpend->TxOutIdx);
				}
			}
			for (int64_t i = 0; i < tx->TxOuts.size(); i++)
			{
				UnspentTxOut::AddToMap(tx->TxOuts[i], tx->Id(), i, tx->IsCoinbase(), chain.size());
			}
		}
	}

	if (!doingReorg && ReorgIfNecessary() or chainIdx == ActiveChainIdx)
	{
		//TODO: interrupt mining
	}

	//TODO: send block to peers

	return chainIdx;
}

std::shared_ptr<Block> Chain::DisconnectBlock(const std::shared_ptr<Block>& block)
{
	std::lock_guard lock(Lock);

	auto back = ActiveChain.back();
	if (block->Id() != back->Id())
		throw std::exception("Block being disconnected must be the tip");

	for (const auto& tx : block->Txs)
	{
		Mempool::Map[tx->Id()] = tx;

		for (const auto& txIn : tx->TxIns)
		{
			if (txIn->ToSpend != nullptr)
			{
				auto [txOut, tx, txOutIdx, isCoinbase, height] = FindTxOutForTxInInActiveChain(txIn);

				UnspentTxOut::AddToMap(txOut, tx->Id(), txOutIdx, isCoinbase, height);
			}
		}
		for (int64_t i = 0; i < tx->TxOuts.size(); i++)
		{
			UnspentTxOut::RemoveFromMap(tx->Id(), i);
		}
	}

	ActiveChain.pop_back();

	return back;
}

int64_t Chain::ValidateBlock(const std::shared_ptr<Block>& block)
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
			return prev_block_chain_idx;
		else if (prev_block->Id() != ActiveChain.back()->Id())
			return prev_block_chain_idx + 1;
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

	return prev_block_chain_idx;
}

void Chain::SaveToDisk()
{
	std::lock_guard lock(Lock);

	//TODO
}

void Chain::LoadFromDisk()
{
	std::lock_guard lock(Lock);

	//TODO
}

int64_t Chain::GetMedianTimePast(size_t numLastBlocks)
{
	std::lock_guard lock(Lock);

	if (numLastBlocks > ActiveChain.size())
		return 0;

	size_t first_idx = ActiveChain.size() - numLastBlocks;
	size_t median_idx = first_idx + (numLastBlocks / 2);
	if (numLastBlocks % 2 == 0)
		median_idx -= 1;

	return ActiveChain[median_idx]->Timestamp;
}

bool Chain::ReorgIfNecessary()
{
	std::lock_guard lock(Lock);

	bool reorged = false;

	auto frozenSideBranches = SideBranches;
	for (int64_t i = 0; i < frozenSideBranches.size(); i++)
	{
		const auto& chain = frozenSideBranches[i];

		auto [fork_block, fork_height, fork_idx] = LocateBlockInActiveChain(chain[0]->PrevBlockHash);

		size_t branchHeight = chain.size() + fork_idx;
		if (branchHeight > GetCurrentHeight())
		{
			reorged |= TryReorg(chain, i, fork_idx);
		}
	}

	return reorged;
}

bool Chain::TryReorg(const std::vector<std::shared_ptr<Block>>& branch, int64_t branchIdx, int64_t forkIdx)
{
	std::lock_guard lock(Lock);

	auto fork_block = ActiveChain[forkIdx];

	auto oldActiveChain = DisconnectToFork(fork_block);

	assert(branch.front()->PrevBlockHash == ActiveChain.back()->Id());

	for (const auto& block : branch)
	{
		int64_t connectedBlockIdx = ConnectBlock(block, true);
		if (connectedBlockIdx != ActiveChainIdx)
		{
			RollbackReorg(oldActiveChain, fork_block);

			return false;
		}
	}

	SideBranches.erase(SideBranches.begin() + branchIdx - 1);
	SideBranches.push_back(oldActiveChain);

	return true;
}

void Chain::RollbackReorg(const std::vector<std::shared_ptr<Block>>& oldActiveChain, const std::shared_ptr<Block>& forkBlock)
{
	std::lock_guard lock(Lock);

	DisconnectToFork(forkBlock);

	for (const auto& block : oldActiveChain)
	{
		auto connectedBlockIdx = ConnectBlock(block, true);

		assert(connectedBlockIdx == ActiveChainIdx);
	}
}

std::vector<std::shared_ptr<Block>> Chain::DisconnectToFork(const std::shared_ptr<Block>& forkBlock)
{
	std::lock_guard lock(Lock);

	std::vector<std::shared_ptr<Block>> disconnected_chain;

	while (ActiveChain.back()->Id() != forkBlock->Id())
	{
		disconnected_chain.push_back(DisconnectBlock(ActiveChain.back()));
	}

	return disconnected_chain;
}

std::tuple<std::shared_ptr<TxOut>, std::shared_ptr<Tx>, int64_t, bool, int64_t> Chain::FindTxOutForTxIn(const std::shared_ptr<TxIn>& txIn, const std::vector<std::shared_ptr<Block>>& chain)
{
	std::lock_guard lock(Lock);

	for (int64_t height = 0; height < chain.size(); height++)
	{
		for (const auto& tx : chain[height]->Txs)
		{
			const auto& toSpend = txIn->ToSpend;
			if (toSpend->TxId == tx->Id())
			{
				const auto& txOut = tx->TxOuts[toSpend->TxOutIdx];

				return { txOut, tx, toSpend->TxOutIdx, tx->IsCoinbase(), height };
			}
		}
	}

	return { nullptr, nullptr, -1, false, -1 };
}

std::tuple<std::shared_ptr<TxOut>, std::shared_ptr<Tx>, int64_t, bool, int64_t> Chain::FindTxOutForTxInInActiveChain(const std::shared_ptr<TxIn>& txIn)
{
	return FindTxOutForTxIn(txIn, ActiveChain);
}
