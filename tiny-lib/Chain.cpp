#include "pch.hpp"
#include "Chain.hpp"

#include <cassert>
#include <exception>
#include <fstream>
#include <ranges>
#include <fmt/format.h>
#include <openssl/bn.h>

#include "BlockInfoMsg.hpp"
#include "Exceptions.hpp"
#include "HashChecker.hpp"
#include "Log.hpp"
#include "Mempool.hpp"
#include "MerkleTree.hpp"
#include "NetClient.hpp"
#include "NetParams.hpp"
#include "PoW.hpp"
#include "TxOutPoint.hpp"
#include "UnspentTxOut.hpp"
#include "Utils.hpp"

const std::shared_ptr<TxIn> Chain::GenesisTxIn = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{0x00},
                                                                        std::vector<uint8_t>(), 0);
const std::shared_ptr<TxOut> Chain::GenesisTxOut = std::make_shared<TxOut>(
	5000000000, "143UVyz7ooiAv1pMqbwPPpnH4BV9ifJGFF");
const std::shared_ptr<Tx> Chain::GenesisTx = std::make_shared<Tx>(std::vector{GenesisTxIn},
                                                                  std::vector{GenesisTxOut}, -1);
const std::shared_ptr<Block> Chain::GenesisBlock = std::make_shared<Block>(
	0, "", "8fdcb01b725d0dba8437ab9fd20714acc5b6ff0ea7a3a052d72318ab234b5d0d",
	1501821412, 24, 4611686018428302567, std::vector{GenesisTx});

std::vector<std::shared_ptr<Block>> Chain::ActiveChain{GenesisBlock};
std::vector<std::vector<std::shared_ptr<Block>>> Chain::SideBranches{};
std::vector<std::shared_ptr<Block>> Chain::OrphanBlocks{};

std::recursive_mutex Chain::Mutex;

std::atomic_bool Chain::InitialBlockDownloadComplete = false;

uint32_t Chain::GetCurrentHeight()
{
	std::lock_guard lock(Mutex);

	return ActiveChain.size();
}

int64_t Chain::GetMedianTimePast(uint32_t numLastBlocks)
{
	std::lock_guard lock(Mutex);

	if (numLastBlocks > ActiveChain.size())
		return 0;

	const uint32_t first_idx = ActiveChain.size() - numLastBlocks;
	uint32_t median_idx = first_idx + (numLastBlocks / 2);
	if (numLastBlocks % 2 == 0)
		median_idx -= 1;

	return ActiveChain[median_idx]->Timestamp;
}

uint32_t Chain::ValidateBlock(const std::shared_ptr<Block>& block)
{
	std::lock_guard lock(Mutex);

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
	auto coinbase_it = std::ranges::find_if(txs, coinbase_pred);
	if (coinbase_it != txs.begin() || std::find_if(++coinbase_it, txs.end(), coinbase_pred) != txs.end())
		throw BlockValidationException("First transaction must be coinbase and no more");

	for (uint32_t i = 0; i < txs.size(); i++)
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

	uint32_t prev_block_chain_idx;
	if (block->PrevBlockHash.empty())
	{
		prev_block_chain_idx = ActiveChainIdx;
	}
	else
	{
		auto [prev_block, prev_block_height, prev_block_chain_idx2] = LocateBlockInAllChains(block->PrevBlockHash);
		if (prev_block == nullptr)
			throw BlockValidationException(
				fmt::format("Previous block {} is not found in any chain", block->PrevBlockHash).c_str(), block);

		if (prev_block_chain_idx2 != ActiveChainIdx)
			return prev_block_chain_idx2;
		if (prev_block->Id() != ActiveChain.back()->Id())
			return prev_block_chain_idx2 + 1;

		prev_block_chain_idx = prev_block_chain_idx2;
	}

	if (PoW::GetNextWorkRequired(block->PrevBlockHash) != block->Bits)
		throw BlockValidationException("Bits are incorrect");

	std::vector nonCoinbaseTxs(block->Txs.begin() + 1, block->Txs.end());
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
			const std::string msg = fmt::format("Transaction {} failed to validate", nonCoinbaseTx->Id());

			LOG_ERROR(msg);

			throw BlockValidationException(msg.c_str());
		}
	}

	return prev_block_chain_idx;
}

int64_t Chain::ConnectBlock(const std::shared_ptr<Block>& block, bool doingReorg /*= false*/)
{
	std::lock_guard lock(Mutex);

	const auto blockId = block->Id();

	std::shared_ptr<Block> located_block;
	if (!doingReorg)
	{
		auto [located_block2, located_block_height, located_block_chain_idx] = LocateBlockInAllChains(block->Id());
		located_block = located_block2;
	}
	else
	{
		auto [located_block2, located_block_height] = LocateBlockInActiveChain(block->Id());
		located_block = located_block2;
	}
	if (located_block != nullptr)
	{
		LOG_TRACE("Ignore already seen block {}", blockId);

		return -1;
	}

	uint32_t chainIdx;
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
			for (uint32_t i = 0; i < tx->TxOuts.size(); i++)
			{
				UTXO::AddToMap(tx->TxOuts[i], txId, i, tx->IsCoinbase(), chain.size());
			}
		}
	}

	if ((!doingReorg && ReorgIfNecessary()) || chainIdx == ActiveChainIdx)
	{
		PoW::MineInterrupt = true;

		LOG_INFO("Block accepted with height {} and txs {}", ActiveChain.size() - 1, block->Txs.size());
	}

	NetClient::SendMsgRandom(BlockInfoMsg(block));

	return chainIdx;
}

std::shared_ptr<Block> Chain::DisconnectBlock(const std::shared_ptr<Block>& block)
{
	std::lock_guard lock(Mutex);

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
		for (uint32_t i = 0; i < tx->TxOuts.size(); i++)
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
	std::lock_guard lock(Mutex);

	std::vector<std::shared_ptr<Block>> disconnected_chain;

	const auto forkBlockId = forkBlock->Id();
	while (ActiveChain.back()->Id() != forkBlockId)
	{
		disconnected_chain.emplace_back(DisconnectBlock(ActiveChain.back()));
	}

	std::ranges::reverse(disconnected_chain);

	return disconnected_chain;
}

bool Chain::ReorgIfNecessary()
{
	std::lock_guard lock(Mutex);

	bool reorged = false;

	auto frozenSideBranches = SideBranches;
	uint32_t branch_idx = 1;
	for (const auto& chain : frozenSideBranches)
	{
		auto [fork_block, fork_heigh] = LocateBlockInActiveChain(chain[0]->PrevBlockHash);

		uint32_t branchHeight = chain.size() + fork_heigh;
		if (branchHeight > GetCurrentHeight())
		{
			LOG_INFO("Attempting reorg of idx {} to active chain, new height of {} vs. {}", branch_idx, branchHeight,
			         fork_heigh);

			reorged |= TryReorg(chain, branch_idx, fork_heigh);
		}
		branch_idx++;
	}

	return reorged;
}

bool Chain::TryReorg(const std::vector<std::shared_ptr<Block>>& branch, uint32_t branchIdx, uint32_t forkIdx)
{
	std::lock_guard lock(Mutex);

	const auto fork_block = ActiveChain[forkIdx];

	const auto oldActiveChain = DisconnectToFork(fork_block);

	assert(branch.front()->PrevBlockHash == ActiveChain.back()->Id());

	for (const auto& block : branch)
	{
		if (ConnectBlock(block, true) != ActiveChainIdx)
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

void Chain::RollbackReorg(const std::vector<std::shared_ptr<Block>>& oldActiveChain,
                          const std::shared_ptr<Block>& forkBlock, uint32_t branchIdx)
{
	std::lock_guard lock(Mutex);

	LOG_INFO("Reorg of idx {} to active chain failed", branchIdx);

	DisconnectToFork(forkBlock);

	for (const auto& block : oldActiveChain)
	{
		const auto connectedBlockIdx = ConnectBlock(block, true);

		assert(connectedBlockIdx == ActiveChainIdx);
	}
}

std::pair<std::shared_ptr<Block>, int64_t> Chain::LocateBlockInChain(const std::string& blockHash,
                                                                     const std::vector<std::shared_ptr<Block>>& chain)
{
	std::lock_guard lock(Mutex);

	uint32_t height = 0;
	for (const auto& block : chain)
	{
		if (block->Id() == blockHash)
		{
			return {block, height};
		}

		height++;
	}

	return {nullptr, -1};
}

std::tuple<std::shared_ptr<Block>, int64_t> Chain::LocateBlockInActiveChain(const std::string& blockHash)
{
	return LocateBlockInChain(blockHash, ActiveChain);
}

std::tuple<std::shared_ptr<Block>, int64_t, int64_t> Chain::LocateBlockInAllChains(const std::string& blockHash)
{
	std::lock_guard lock(Mutex);

	uint32_t chain_idx = 0;
	auto [located_block, located_block_height] = LocateBlockInActiveChain(blockHash);
	if (located_block != nullptr)
		return {located_block, located_block_height, chain_idx};
	chain_idx++;

	for (const auto& side_chain : SideBranches)
	{
		auto [located_block, located_block_height] = LocateBlockInChain(blockHash, side_chain);
		if (located_block != nullptr)
			return {located_block, located_block_height, chain_idx};
		chain_idx++;
	}

	return {nullptr, -1, -1};
}

std::tuple<std::shared_ptr<TxOut>, std::shared_ptr<Tx>, int64_t, bool, int64_t> Chain::FindTxOutForTxIn(
	const std::shared_ptr<TxIn>& txIn, const std::vector<std::shared_ptr<Block>>& chain)
{
	std::lock_guard lock(Mutex);

	for (uint32_t height = 0; height < chain.size(); height++)
	{
		for (const auto& tx : chain[height]->Txs)
		{
			const auto& toSpend = txIn->ToSpend;
			if (toSpend->TxId == tx->Id())
			{
				const auto& txOut = tx->TxOuts[toSpend->TxOutIdx];

				return {txOut, tx, toSpend->TxOutIdx, tx->IsCoinbase(), height};
			}
		}
	}

	return {nullptr, nullptr, -1, false, -1};
}

std::tuple<std::shared_ptr<TxOut>, std::shared_ptr<Tx>, int64_t, bool, int64_t> Chain::FindTxOutForTxInInActiveChain(
	const std::shared_ptr<TxIn>& txIn)
{
	return FindTxOutForTxIn(txIn, ActiveChain);
}

void Chain::SaveToDisk()
{
	std::lock_guard lock(Mutex);

	LOG_INFO("Saving chain with {} blocks", ActiveChain.size());

	//TODO: append from previously saved height
	std::ofstream chain_out(ChainPath, std::ios::binary | std::ios::trunc);
	BinaryBuffer chainData;
	chainData.WriteSize(ActiveChain.size() - 1);
	for (uint32_t height = 1; height < ActiveChain.size(); height++)
	{
		chainData.WriteRaw(ActiveChain[height]->Serialize().GetBuffer());
	}
	auto& chainDataBuffer = chainData.GetBuffer();
	chain_out.write(reinterpret_cast<const char*>(chainDataBuffer.data()), chainDataBuffer.size());
	chain_out.flush();
	chain_out.close();
}

bool Chain::LoadFromDisk()
{
	std::lock_guard lock(Mutex);

	std::ifstream chain_in(ChainPath, std::ios::binary);
	if (chain_in.good())
	{
		BinaryBuffer chainData(std::vector<uint8_t>(std::istreambuf_iterator(chain_in), {}));
		uint32_t blockSize = 0;
		if (chainData.ReadSize(blockSize))
		{
			std::vector<std::shared_ptr<Block>> loadedChain;
			loadedChain.reserve(blockSize);
			for (uint32_t i = 0; i < blockSize; i++)
			{
				auto block = std::make_shared<Block>();
				if (!block->Deserialize(chainData))
				{
					chain_in.close();
					LOG_ERROR("Load chain failed, starting from genesis");

					return false;
				}
				loadedChain.push_back(block);
			}
			for (const auto& block : loadedChain)
			{
				if (ConnectBlock(block) != ActiveChainIdx)
				{
					chain_in.close();
					ActiveChain.clear();
					ActiveChain.push_back(GenesisBlock);
					SideBranches.clear();
					UTXO::Map.clear();
					Mempool::Map.clear();

					LOG_ERROR("Load chain failed, starting from genesis");

					return false;
				}
			}
			chain_in.close();
			LOG_INFO("Loaded chain with {} blocks", ActiveChain.size());

			return true;
		}
		LOG_ERROR("Load chain failed, starting from genesis");

		return false;
	}
	LOG_ERROR("Load chain failed, starting from genesis");

	return false;
}
