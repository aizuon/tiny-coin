#include "pch.hpp"
#include "Chain.hpp"

#include <cassert>
#include <exception>
#include <fstream>
#include <limits>
#include <ranges>
#include <fmt/format.h>

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
#include "uint256_t.hpp"
#include "UnspentTxOut.hpp"
#include "Utils.hpp"

const std::shared_ptr<TxIn> Chain::GenesisTxIn = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(),
                                                                        std::vector<uint8_t>(), -1);
const std::shared_ptr<TxOut> Chain::GenesisTxOut = std::make_shared<TxOut>(
	5000000000, "143UVyz7ooiAv1pMqbwPPpnH4BV9ifJGFF");
const std::shared_ptr<Tx> Chain::GenesisTx = std::make_shared<Tx>(std::vector{ GenesisTxIn },
                                                                  std::vector{ GenesisTxOut }, 0);
const std::shared_ptr<Block> Chain::GenesisBlock = std::make_shared<Block>(
	0, "", "75b7747cdbad68d5e40269399d9d8d6c048cc80a9e1b355379a5ed831ffbc1a8",
	1501821412, 24, 13835058055287124368, std::vector{ GenesisTx });

std::vector<std::shared_ptr<Block>> Chain::ActiveChain{ GenesisBlock };
std::vector<std::vector<std::shared_ptr<Block>>> Chain::SideBranches{};
std::vector<std::shared_ptr<Block>> Chain::OrphanBlocks{};

std::recursive_mutex Chain::Mutex;

std::atomic_bool Chain::InitialBlockDownloadComplete = false;

uint32_t Chain::GetCurrentHeight()
{
	std::scoped_lock lock(Mutex);

	return ActiveChain.size();
}

int64_t Chain::GetMedianTimePast(uint32_t num_last_blocks)
{
	std::scoped_lock lock(Mutex);

	if (num_last_blocks > ActiveChain.size())
		return 0;

	const uint32_t first_idx = ActiveChain.size() - num_last_blocks;
	uint32_t median_idx = first_idx + num_last_blocks / 2;
	if (num_last_blocks % 2 == 0)
		median_idx -= 1;

	return ActiveChain[median_idx]->Timestamp;
}

uint32_t Chain::ValidateBlock(std::shared_ptr<Block> block)
{
	std::scoped_lock lock(Mutex);

	const auto& txs = block->Txs;

	if (txs.empty())
		throw BlockValidationException("Transactions empty");

	if (block->Timestamp - Utils::GetUnixTimestamp() > static_cast<int64_t>(NetParams::MAX_FUTURE_BLOCK_TIME_IN_SECS))
		throw BlockValidationException("Block timestamp too far in future");

	const uint256_t target_hash = uint256_t(1) << (std::numeric_limits<uint8_t>::max() - block->Bits);
	if (!HashChecker::IsValid(block->Id(), target_hash))
		throw BlockValidationException("Block header does not satisfy bits");

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
		catch (const TxValidationException& ex)
		{
			LOG_ERROR(ex.what());

			LOG_ERROR("Transaction {} in block {} failed validation", txs[i]->Id(), block->Id());

			throw BlockValidationException(fmt::format("Transaction {} invalid", txs[i]->Id()).c_str());
		}
	}

	if (MerkleTree::GetRootOfTxs(txs)->Value != block->MerkleHash)
		throw BlockValidationException("Merkle hash invalid");

	if (block->Timestamp <= GetMedianTimePast(11))
		throw BlockValidationException("Timestamp too old");

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
				fmt::format("Previous block {} not found in any chain", block->PrevBlockHash).c_str(), block);

		if (prev_block_chain_idx2 != ActiveChainIdx)
			return prev_block_chain_idx2;
		if (prev_block->Id() != ActiveChain.back()->Id())
			return prev_block_chain_idx2 + 1;

		prev_block_chain_idx = prev_block_chain_idx2;
	}

	if (PoW::GetNextWorkRequired(block->PrevBlockHash) != block->Bits)
		throw BlockValidationException("Bits incorrect");

	const std::vector non_coinbase_txs(block->Txs.begin() + 1, block->Txs.end());
	Tx::ValidateRequest req;
	req.SiblingsInBlock = non_coinbase_txs;
	req.Allow_UTXO_FromMempool = false;
	for (const auto& non_coinbase_tx : non_coinbase_txs)
	{
		try
		{
			non_coinbase_tx->Validate(req);
		}
		catch (const TxValidationException& ex)
		{
			LOG_ERROR(ex.what());

			const std::string msg = fmt::format("Transaction {} failed to validate", non_coinbase_tx->Id());

			LOG_ERROR(msg);

			throw BlockValidationException(msg.c_str());
		}
	}

	return prev_block_chain_idx;
}

int64_t Chain::ConnectBlock(std::shared_ptr<Block> block, bool doing_reorg /*= false*/)
{
	std::scoped_lock lock(Mutex);

	const auto block_id = block->Id();

	std::shared_ptr<Block> located_block;
	if (!doing_reorg)
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
		LOG_INFO("Ignore already seen block {}", block_id);

		return -1;
	}

	uint32_t chain_idx;
	try
	{
		chain_idx = ValidateBlock(block);
	}
	catch (const BlockValidationException& ex)
	{
		LOG_ERROR(ex.what());

		LOG_ERROR("Block {} failed validation", block_id);
		if (ex.ToOrphan != nullptr)
		{
			LOG_INFO("Found orphan block {}", block_id);

			OrphanBlocks.push_back(ex.ToOrphan);
		}

		return -1;
	}

	if (chain_idx != ActiveChainIdx && SideBranches.size() < chain_idx)
	{
		LOG_INFO("Creating a new side branch with idx {} for block {}", chain_idx, block_id);

		SideBranches.emplace_back();
	}

	LOG_INFO("Connecting block {} to chain {}", block_id, chain_idx);

	auto& chain = chain_idx == ActiveChainIdx ? ActiveChain : SideBranches[chain_idx - 1];
	chain.push_back(block);

	if (chain_idx == ActiveChainIdx)
	{
		for (const auto& tx : block->Txs)
		{
			const auto tx_id = tx->Id();

			{
				std::scoped_lock lock_mempool(Mempool::Mutex);

				if (Mempool::Map.contains(tx_id))
					Mempool::Map.erase(tx_id);
			}

			if (!tx->IsCoinbase())
			{
				for (const auto& tx_in : tx->TxIns)
				{
					UTXO::RemoveFromMap(tx_in->ToSpend->TxId, tx_in->ToSpend->TxOutIdx);
				}
			}
			for (uint32_t i = 0; i < tx->TxOuts.size(); i++)
			{
				UTXO::AddToMap(tx->TxOuts[i], tx_id, i, tx->IsCoinbase(), chain.size());
			}
		}
	}

	if (!doing_reorg && ReorgIfNecessary() || chain_idx == ActiveChainIdx)
	{
		PoW::MineInterrupt = true;

		LOG_INFO("Block accepted at height {} with {} txs", ActiveChain.size() - 1, block->Txs.size());
	}

	NetClient::SendMsgRandom(BlockInfoMsg(block));

	return chain_idx;
}

std::shared_ptr<Block> Chain::DisconnectBlock(std::shared_ptr<Block> block)
{
	std::scoped_lock lock(Mutex);

	const auto block_id = block->Id();

	auto back = ActiveChain.back();
	if (block_id != back->Id())
		throw std::exception("Block being disconnected must be the tip");

	for (const auto& tx : block->Txs)
	{
		const auto tx_id = tx->Id();

		{
			std::scoped_lock lock_mempool(Mempool::Mutex);

			Mempool::Map[tx_id] = tx;
		}

		for (const auto& tx_in : tx->TxIns)
		{
			if (tx_in->ToSpend != nullptr)
			{
				auto [tx_out, tx, tx_out_idx, is_coinbase, height] = FindTxOutForTxInInActiveChain(tx_in);

				UTXO::AddToMap(tx_out, tx_id, tx_out_idx, is_coinbase, height);
			}
		}
		for (uint32_t i = 0; i < tx->TxOuts.size(); i++)
		{
			UTXO::RemoveFromMap(tx_id, i);
		}
	}

	ActiveChain.pop_back();

	LOG_INFO("Block {} disconnected", block_id);

	return back;
}

std::vector<std::shared_ptr<Block>> Chain::DisconnectToFork(std::shared_ptr<Block> fork_block)
{
	std::scoped_lock lock(Mutex);

	std::vector<std::shared_ptr<Block>> disconnected_chain;

	const auto fork_block_id = fork_block->Id();
	while (ActiveChain.back()->Id() != fork_block_id)
	{
		disconnected_chain.emplace_back(DisconnectBlock(ActiveChain.back()));
	}

	std::ranges::reverse(disconnected_chain);

	return disconnected_chain;
}

bool Chain::ReorgIfNecessary()
{
	std::scoped_lock lock(Mutex);

	bool reorged = false;

	const auto frozen_side_branches = SideBranches;
	uint32_t branch_idx = 1;
	for (const auto& chain : frozen_side_branches)
	{
		auto [fork_block, fork_height] = LocateBlockInActiveChain(chain[0]->PrevBlockHash);

		uint32_t branch_height = chain.size() + fork_height;
		if (branch_height > GetCurrentHeight())
		{
			LOG_INFO("Attempting reorg of idx {} to active chain, new height of {} vs. {}", branch_idx, branch_height,
			         fork_height);

			reorged |= TryReorg(chain, branch_idx, fork_height);
		}
		branch_idx++;
	}

	return reorged;
}

bool Chain::TryReorg(const std::vector<std::shared_ptr<Block>>& branch, uint32_t branch_idx, uint32_t fork_idx)
{
	std::scoped_lock lock(Mutex);

	const auto fork_block = ActiveChain[fork_idx];

	const auto old_active_chain = DisconnectToFork(fork_block);

	assert(branch.front()->PrevBlockHash == ActiveChain.back()->Id());

	for (const auto& block : branch)
	{
		if (ConnectBlock(block, true) != ActiveChainIdx)
		{
			RollbackReorg(old_active_chain, fork_block, branch_idx);

			return false;
		}
	}

	SideBranches.erase(SideBranches.begin() + branch_idx - 1);
	SideBranches.push_back(old_active_chain);

	LOG_INFO("Chain reorganized, new height {} with tip {}", ActiveChain.size(), ActiveChain.back()->Id());

	return true;
}

void Chain::RollbackReorg(const std::vector<std::shared_ptr<Block>>& old_active_chain,
                          std::shared_ptr<Block> fork_block, uint32_t branch_idx)
{
	std::scoped_lock lock(Mutex);

	LOG_ERROR("Reorg of idx {} to active chain failed", branch_idx);

	DisconnectToFork(fork_block);

	for (const auto& block : old_active_chain)
	{
		const auto connected_block_idx = ConnectBlock(block, true);

		assert(connected_block_idx == ActiveChainIdx);
	}
}

std::pair<std::shared_ptr<Block>, int64_t> Chain::LocateBlockInChain(const std::string& block_hash,
                                                                     const std::vector<std::shared_ptr<Block>>& chain)
{
	std::scoped_lock lock(Mutex);

	uint32_t height = 0;
	for (const auto& block : chain)
	{
		if (block->Id() == block_hash)
		{
			return { block, height };
		}

		height++;
	}

	return { nullptr, -1 };
}

std::tuple<std::shared_ptr<Block>, int64_t> Chain::LocateBlockInActiveChain(const std::string& block_hash)
{
	return LocateBlockInChain(block_hash, ActiveChain);
}

std::tuple<std::shared_ptr<Block>, int64_t, int64_t> Chain::LocateBlockInAllChains(const std::string& block_hash)
{
	std::scoped_lock lock(Mutex);

	uint32_t chain_idx = 0;
	auto [located_block, located_block_height] = LocateBlockInActiveChain(block_hash);
	if (located_block != nullptr)
		return { located_block, located_block_height, chain_idx };
	chain_idx++;

	for (const auto& side_chain : SideBranches)
	{
		auto [located_block, located_block_height] = LocateBlockInChain(block_hash, side_chain);
		if (located_block != nullptr)
			return { located_block, located_block_height, chain_idx };
		chain_idx++;
	}

	return { nullptr, -1, -1 };
}

std::tuple<std::shared_ptr<TxOut>, std::shared_ptr<Tx>, int64_t, bool, int64_t> Chain::FindTxOutForTxIn(
	std::shared_ptr<TxIn> tx_in, const std::vector<std::shared_ptr<Block>>& chain)
{
	std::scoped_lock lock(Mutex);

	for (uint32_t height = 0; height < chain.size(); height++)
	{
		for (const auto& tx : chain[height]->Txs)
		{
			const auto& to_spend = tx_in->ToSpend;
			if (to_spend->TxId == tx->Id())
			{
				const auto& tx_out = tx->TxOuts[to_spend->TxOutIdx];

				return { tx_out, tx, to_spend->TxOutIdx, tx->IsCoinbase(), height };
			}
		}
	}

	return { nullptr, nullptr, -1, false, -1 };
}

std::tuple<std::shared_ptr<TxOut>, std::shared_ptr<Tx>, int64_t, bool, int64_t> Chain::FindTxOutForTxInInActiveChain(
	std::shared_ptr<TxIn> tx_in)
{
	return FindTxOutForTxIn(tx_in, ActiveChain);
}

void Chain::SaveToDisk()
{
	std::scoped_lock lock(Mutex);

	LOG_INFO("Saving chain with {} blocks", ActiveChain.size());

	//TODO: append from previously saved height
	std::ofstream chain_out(ChainPath, std::ios::binary | std::ios::trunc);
	BinaryBuffer chain_data;
	chain_data.WriteSize(ActiveChain.size() - 1);
	for (uint32_t height = 1; height < ActiveChain.size(); height++)
	{
		chain_data.WriteRaw(ActiveChain[height]->Serialize().GetBuffer());
	}
	auto& chain_data_buffer = chain_data.GetBuffer();
	chain_out.write(reinterpret_cast<const char*>(chain_data_buffer.data()), chain_data_buffer.size());
	chain_out.flush();
	chain_out.close();
}

bool Chain::LoadFromDisk()
{
	std::scoped_lock lock(Mutex);

	std::ifstream chain_in(ChainPath, std::ios::binary);
	if (chain_in.good())
	{
		BinaryBuffer chain_data(std::vector<uint8_t>(std::istreambuf_iterator(chain_in), {}));
		uint32_t block_size = 0;
		if (chain_data.ReadSize(block_size))
		{
			std::vector<std::shared_ptr<Block>> loaded_chain;
			loaded_chain.reserve(block_size);
			for (uint32_t i = 0; i < block_size; i++)
			{
				auto block = std::make_shared<Block>();
				if (!block->Deserialize(chain_data))
				{
					chain_in.close();
					LOG_ERROR("Load chain failed, starting from genesis");

					return false;
				}
				loaded_chain.push_back(block);
			}
			for (const auto& block : loaded_chain)
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
