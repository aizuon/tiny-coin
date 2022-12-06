#include "pch.hpp"
#include "Mempool.hpp"

#include <ranges>
#include <utility>

#include "BinaryBuffer.hpp"
#include "Exceptions.hpp"
#include "Log.hpp"
#include "NetClient.hpp"
#include "NetParams.hpp"
#include "PoW.hpp"
#include "TxInfoMsg.hpp"

std::unordered_map<std::string, std::shared_ptr<Tx>> Mempool::Map;

std::vector<std::shared_ptr<Tx>> Mempool::OrphanedTxs;

std::recursive_mutex Mempool::Mutex;

std::shared_ptr<UTXO> Mempool::Find_UTXO_InMempool(std::shared_ptr<TxOutPoint> tx_out_point)
{
	std::scoped_lock lock(Mutex);

	if (!Map.contains(tx_out_point->TxId))
		return nullptr;

	const auto& tx = Map[tx_out_point->TxId];
	if (tx->TxOuts.size() - 1 < tx_out_point->TxOutIdx)
	{
		LOG_ERROR("Unable to find UTXO in mempool for {}", tx_out_point->TxId);

		return nullptr;
	}

	const auto& tx_out = tx->TxOuts[tx_out_point->TxOutIdx];

	return std::make_shared<UTXO>(tx_out, tx_out_point, false, -1);
}

std::shared_ptr<Block> Mempool::SelectFromMempool(std::shared_ptr<Block> block)
{
	std::scoped_lock lock(Mutex);

	auto new_block = std::make_shared<Block>(*block);

	std::vector<std::pair<std::string, std::shared_ptr<Tx>>> map_vector(Map.begin(), Map.end());
	std::ranges::sort(map_vector,
	                  [](const std::pair<std::string, std::shared_ptr<Tx>>& a,
	                     const std::pair<std::string, std::shared_ptr<Tx>>& b) -> bool
	                  {
		                  const auto& [a_txId, a_tx] = a;
		                  const auto& [b_txId, b_tx] = b;
		                  return PoW::CalculateFees(a_tx) < PoW::CalculateFees(b_tx);
	                  });

	std::set<std::string> added_to_block;
	for (const auto& tx_id : map_vector | std::views::keys)
		new_block = TryAddToBlock(new_block, tx_id, added_to_block);

	return new_block;
}

void Mempool::AddTxToMempool(std::shared_ptr<Tx> tx)
{
	std::scoped_lock lock(Mutex);

	const auto tx_id = tx->Id();
	if (Map.contains(tx_id))
	{
		LOG_INFO("Transaction {} already seen", tx_id);

		return;
	}

	try
	{
		tx->Validate(Tx::ValidateRequest());
	}
	catch (const TxValidationException& ex)
	{
		LOG_ERROR(ex.what());

		if (ex.ToOrphan != nullptr)
		{
			LOG_INFO("Transaction {} submitted as orphan", ex.ToOrphan->Id());

			OrphanedTxs.push_back(ex.ToOrphan);

			return;
		}
		LOG_ERROR("Transaction {} rejected", tx_id);

		return;
	}

	Map[tx_id] = tx;

	LOG_TRACE("Transaction {} added to mempool", tx_id);

	NetClient::SendMsgRandom(TxInfoMsg(tx));
}

bool Mempool::CheckBlockSize(std::shared_ptr<Block> block)
{
	return block->Serialize().GetSize() < NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES;
}

std::shared_ptr<Block> Mempool::TryAddToBlock(std::shared_ptr<Block> block, const std::string& tx_id,
                                              std::set<std::string>& added_to_block)
{
	std::scoped_lock lock(Mutex);

	if (added_to_block.contains(tx_id))
		return block;

	if (!Map.contains(tx_id))
		return block;

	const auto& tx = Map[tx_id];

	for (const auto& tx_in : tx->TxIns)
	{
		const auto& to_spend = tx_in->ToSpend;

		if (UTXO::FindInMap(to_spend) != nullptr)
			continue;

		const auto& in_mempool = Find_UTXO_InMempool(to_spend);
		if (in_mempool == nullptr)
		{
			LOG_ERROR("Unable to find UTXO for {}", tx_in->ToSpend->TxId);

			return nullptr;
		}

		block = TryAddToBlock(block, in_mempool->TxOutPoint->TxId, added_to_block);
		if (block == nullptr)
		{
			LOG_ERROR("Unable to add parent");

			return nullptr;
		}
	}

	auto new_block = std::make_shared<Block>(*block);
	const auto& block_txs = block->Txs;
	std::vector<std::shared_ptr<Tx>> txs;
	txs.insert(txs.end(), block_txs.begin(), block_txs.end());
	txs.push_back(tx);
	new_block->Txs = txs;

	if (CheckBlockSize(new_block))
	{
		LOG_TRACE("Added transaction {} to block {}", tx_id, block->Id());

		added_to_block.insert(tx_id);

		return new_block;
	}
	return block;
}
