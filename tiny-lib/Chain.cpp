#include "pch.hpp"

#include <cassert>
#include <exception>
#include <algorithm>
#include <fstream>
#include <fmt/format.h>
#include <openssl/bn.h>

#include "Chain.hpp"
#include "Exceptions.hpp"
#include "Log.hpp"
#include "NetParams.hpp"
#include "Utils.hpp"
#include "Mempool.hpp"
#include "MerkleTree.hpp"
#include "PoW.hpp"
#include "HashChecker.hpp"
#include "SHA256.hpp"
#include "BlockInfoMsg.hpp"
#include "NetClient.hpp"
#include "TxOutPoint.hpp"
#include "UnspentTxOut.hpp"

const std::shared_ptr<TxIn> Chain::GenesisTxIn = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{ 0x00 }, std::vector<uint8_t>(), 0);
const std::shared_ptr<TxOut> Chain::GenesisTxOut = std::make_shared<TxOut>(5000000000, "143UVyz7ooiAv1pMqbwPPpnH4BV9ifJGFF");
const std::shared_ptr<Tx> Chain::GenesisTx = std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ Chain::GenesisTxIn },
	std::vector<std::shared_ptr<TxOut>>{ Chain::GenesisTxOut }, 0);
const std::shared_ptr<Block> Chain::GenesisBlock = std::make_shared<Block>(0, "", "7118894203235a955a908c0abfc6d8fe6edec47b0a04ce1bf7263da3b4366d22",
	1501821412, 24, 10126761, std::vector<std::shared_ptr<Tx>>{ GenesisTx });

std::vector<std::shared_ptr<Block>> Chain::ActiveChain{ GenesisBlock };
std::vector<std::vector<std::shared_ptr<Block>>> Chain::SideBranches{ };
std::vector<std::shared_ptr<Block>> Chain::OrphanBlocks{ };

std::recursive_mutex Chain::Lock;

std::atomic_bool Chain::InitialBlockDownloadComplete = false;

int64_t Chain::GetCurrentHeight()
{
	std::lock_guard lock(Lock);

	return ActiveChain.size();
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

int64_t Chain::ValidateBlock(const std::shared_ptr<Block>& block)
{
	std::lock_guard lock(Lock);

	const auto& txs = block->Txs;

	if (txs.empty())
		throw BlockValidationException("Transactions are empty");

	if (block->Timestamp - Utils::GetUnixTimestamp() > static_cast<int64_t>(NetParams::MAX_FUTURE_BLOCK_TIME_IN_SECS))
		throw BlockValidationException("Block timestamp is too far in future");

	BIGNUM* target_bn = HashChecker::TargetBitsToBN(block->Bits);
	if (!HashChecker::IsValid(block->Id(), target_bn))
	{
		BN_free(target_bn);

		throw BlockValidationException("Block header does not satisfy bits");
	}
	BN_free(target_bn);

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
			LOG_ERROR("Transaction {} in block {} failed to validate", txs[i]->Id(), block->Id());

			throw BlockValidationException(fmt::format("Transaction {} is invalid", txs[i]->Id()).c_str());
		}
	}

	if (MerkleTree::GetRootOfTxs(txs)->Value != block->MerkleHash)
		throw BlockValidationException("Merkle hash is invalid");

	if (block->Timestamp <= GetMedianTimePast(11))
		throw BlockValidationException("Timestamp is too old");

	int64_t prev_block_chain_idx = -1;
	if (block->PrevBlockHash.empty())
	{
		prev_block_chain_idx = ActiveChainIdx;
	}
	else
	{
		auto [prev_block, prev_block_height, prev_block_chain_idx2] = LocateBlockInAllChains(block->PrevBlockHash);
		if (prev_block == nullptr)
			throw BlockValidationException(fmt::format("Previous block {} is not found in any chain", block->PrevBlockHash).c_str(), block);

		if (prev_block_chain_idx2 != ActiveChainIdx)
			return prev_block_chain_idx2;
		else if (prev_block->Id() != ActiveChain.back()->Id())
			return prev_block_chain_idx2 + 1;

		prev_block_chain_idx = prev_block_chain_idx2;
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
			std::string msg = fmt::format("Transaction {} failed to validate", nonCoinbaseTx->Id());

			LOG_ERROR(msg);

			throw BlockValidationException(msg.c_str());
		}
	}

	return prev_block_chain_idx;
}

int64_t Chain::ConnectBlock(const std::shared_ptr<Block>& block, bool doingReorg /*= false*/)
{
	std::lock_guard lock(Lock);

	const auto blockId = block->Id();

	if (!doingReorg)
	{
		auto [located_block, located_block_height, located_block_chain_idx] = LocateBlockInAllChains(block->Id());
		if (located_block != nullptr)
		{
			LOG_TRACE("Ignore already seen block {}", blockId);

			return -1;
		}
	}
	else
	{
		auto [located_block, located_block_height] = LocateBlockInActiveChain(block->Id());
		if (located_block != nullptr)
		{
			LOG_TRACE("Ignore already seen block {}", blockId);

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
		LOG_ERROR("Block {} failed validation", blockId);
		if (ex.ToOrphan != nullptr)
		{
			LOG_INFO("Saw orphan block {}", blockId);

			OrphanBlocks.push_back(ex.ToOrphan);
		}

		return -1;
	}

	if (chainIdx != ActiveChainIdx && SideBranches.size() < chainIdx)
	{
		LOG_INFO("Creating a new side branch {} for block {}", chainIdx, blockId);

		SideBranches.emplace_back();
	}

	LOG_INFO("Connecting block {} to chain {}", blockId, chainIdx);

	auto& chain = chainIdx == ActiveChainIdx ? ActiveChain : SideBranches[chainIdx - 1];
	chain.push_back(block);

	if (chainIdx == ActiveChainIdx)
	{
		for (const auto& tx : block->Txs)
		{
			const auto txId = tx->Id();

			if (Mempool::Map.contains(txId))
				Mempool::Map.erase(txId);

			if (!tx->IsCoinbase())
			{
				for (const auto& txIn : tx->TxIns)
				{
					UTXO::RemoveFromMap(txIn->ToSpend->TxId, txIn->ToSpend->TxOutIdx);
				}
			}
			for (int64_t i = 0; i < tx->TxOuts.size(); i++)
			{
				UTXO::AddToMap(tx->TxOuts[i], txId, i, tx->IsCoinbase(), chain.size());
			}
		}
	}

	if (!doingReorg && ReorgIfNecessary() or chainIdx == ActiveChainIdx)
	{
		PoW::MineInterrupt = true;

		LOG_INFO("Block accepted with height {} and txs {}", ActiveChain.size() - 1, block->Txs.size());
	}

	NetClient::BroadcastMsgAsync(BlockInfoMsg(block));

	return chainIdx;
}

std::shared_ptr<Block> Chain::DisconnectBlock(const std::shared_ptr<Block>& block)
{
	std::lock_guard lock(Lock);

	const auto blockId = block->Id();

	auto back = ActiveChain.back();
	if (blockId != back->Id())
		throw std::exception("Block being disconnected must be the tip");

	for (const auto& tx : block->Txs)
	{
		const auto txId = tx->Id();

		Mempool::Map[txId] = tx;

		for (const auto& txIn : tx->TxIns)
		{
			if (txIn->ToSpend != nullptr)
			{
				auto [txOut, tx, txOutIdx, isCoinbase, height] = FindTxOutForTxInInActiveChain(txIn);

				UTXO::AddToMap(txOut, txId, txOutIdx, isCoinbase, height);
			}
		}
		for (int64_t i = 0; i < tx->TxOuts.size(); i++)
		{
			UTXO::RemoveFromMap(txId, i);
		}
	}

	ActiveChain.pop_back();

	LOG_INFO("Block {} disconnected", blockId);

	return back;
}

std::vector<std::shared_ptr<Block>> Chain::DisconnectToFork(const std::shared_ptr<Block>& forkBlock)
{
	std::lock_guard lock(Lock);

	std::vector<std::shared_ptr<Block>> disconnected_chain;

	const auto forkBlockId = forkBlock->Id();
	while (ActiveChain.back()->Id() != forkBlockId)
	{
		disconnected_chain.push_back(DisconnectBlock(ActiveChain.back()));
	}

	std::reverse(disconnected_chain.begin(), disconnected_chain.end());

	return disconnected_chain;
}

bool Chain::ReorgIfNecessary()
{
	std::lock_guard lock(Lock);

	bool reorged = false;

	auto frozenSideBranches = SideBranches;
	int64_t branch_idx = 1;
	for (const auto& chain : frozenSideBranches)
	{
		auto [fork_block, fork_heigh] = LocateBlockInActiveChain(chain[0]->PrevBlockHash);

		size_t branchHeight = chain.size() + fork_heigh;
		if (branchHeight > GetCurrentHeight())
		{
			LOG_INFO("Attempting reorg of idx {} to active chain, new height of {} vs. {}", branch_idx, branchHeight, fork_heigh);

			reorged |= TryReorg(chain, branch_idx, fork_heigh);
		}
		branch_idx++;
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
			RollbackReorg(oldActiveChain, fork_block, branchIdx);

			return false;
		}
	}

	SideBranches.erase(SideBranches.begin() + branchIdx - 1);
	SideBranches.push_back(oldActiveChain);

	LOG_INFO("Chain reorg -> new height {}, tip {}", ActiveChain.size(), ActiveChain.back()->Id());

	return true;
}

void Chain::RollbackReorg(const std::vector<std::shared_ptr<Block>>& oldActiveChain, const std::shared_ptr<Block>& forkBlock, int64_t branchIdx)
{
	std::lock_guard lock(Lock);

	LOG_INFO("Reorg of idx {} to active chain failed", branchIdx);

	DisconnectToFork(forkBlock);

	for (const auto& block : oldActiveChain)
	{
		auto connectedBlockIdx = ConnectBlock(block, true);

		assert(connectedBlockIdx == ActiveChainIdx);
	}
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

std::tuple<std::shared_ptr<Block>, int64_t> Chain::LocateBlockInActiveChain(const std::string& blockHash)
{
	auto [located_block, located_block_height] = LocateBlockInChain(blockHash, ActiveChain);
	if (located_block != nullptr)
		return { located_block, located_block_height };

	return { located_block, located_block_height };
}

std::tuple<std::shared_ptr<Block>, int64_t, int64_t> Chain::LocateBlockInAllChains(const std::string& blockHash)
{
	std::lock_guard lock(Lock);

	int64_t chain_idx = 0;
	auto [located_block, located_block_height] = LocateBlockInActiveChain(blockHash);
	if (located_block != nullptr)
		return { located_block, located_block_height, chain_idx };
	chain_idx++;

	for (const auto& side_chain : SideBranches)
	{
		auto [located_block, located_block_height] = LocateBlockInChain(blockHash, side_chain);
		if (located_block != nullptr)
			return { located_block, located_block_height, chain_idx };
		chain_idx++;
	}

	return { nullptr, -1, -1 };
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

void Chain::SaveToDisk()
{
	std::lock_guard lock(Lock);

	LOG_INFO("Saving chain with {} blocks", ActiveChain.size());

	std::ofstream chain_out(ChainPath, std::ios::binary | std::ios::trunc);
	BinaryBuffer chainData;
	chainData.Write(ActiveChain.size());
	for (const auto& block : ActiveChain)
	{
		chainData.WriteRaw(block->Serialize().GetWritableBuffer());
	}
	auto& chainDataBuffer = chainData.GetBuffer();
	chain_out.write((const char*)chainDataBuffer.data(), chainDataBuffer.size());
	chain_out.flush();
	chain_out.close();
}

void Chain::LoadFromDisk()
{
	std::lock_guard lock(Lock);

	std::ifstream chain_in(ChainPath, std::ios::binary);
	if (chain_in.good())
	{
		BinaryBuffer chainData(std::vector<uint8_t>(std::istreambuf_iterator<char>(chain_in), {}));
		size_t blockSize = 0;
		if (!chainData.Read(blockSize))
		{
			LOG_ERROR("Load chain failed, starting from genesis");

			return;
		}
		for (size_t i = 0; i < blockSize; i++)
		{
			auto block = std::make_shared<Block>();
			if (!block->Deserialize(chainData))
			{
				LOG_ERROR("Load chain failed, starting from genesis");
				ActiveChain.clear();

				return;
			}
			ActiveChain.push_back(block);
		}
		ActiveChain.clear();
		chain_in.close();
	}
	else
	{
		LOG_ERROR("Load chain failed, starting from genesis");
	}
}
