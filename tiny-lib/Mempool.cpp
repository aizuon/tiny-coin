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

std::shared_ptr<UTXO> Mempool::Find_UTXO_InMempool(const std::shared_ptr<TxOutPoint>& txOutPoint)
{
	std::scoped_lock lock(Mutex);

	if (!Map.contains(txOutPoint->TxId))
		return nullptr;

	const auto& tx = Map[txOutPoint->TxId];
	if (tx->TxOuts.size() - 1 < txOutPoint->TxOutIdx)
	{
		LOG_ERROR("Unable to find UTXO in mempool for {}", txOutPoint->TxId);

		return nullptr;
	}

	const auto& txOut = tx->TxOuts[txOutPoint->TxOutIdx];

	return std::make_shared<UTXO>(txOut, txOutPoint, false, -1);
}

std::shared_ptr<Block> Mempool::SelectFromMempool(const std::shared_ptr<Block>& block)
{
	std::scoped_lock lock(Mutex);

	auto newBlock = std::make_shared<Block>(*block);

	std::vector<std::pair<std::string, std::shared_ptr<Tx>>> map_vector(Map.begin(), Map.end());
	std::ranges::sort(map_vector,
	                  [](const std::pair<std::string, std::shared_ptr<Tx>>& a,
	                     const std::pair<std::string, std::shared_ptr<Tx>>& b) -> bool
	                  {
		                  const auto& [a_txId, a_tx] = a;
		                  const auto& [b_txId, b_tx] = b;
		                  return PoW::CalculateFees(a_tx) < PoW::CalculateFees(b_tx);
	                  });

	std::set<std::string> addedToBlock;
	for (const auto& txId : map_vector | std::views::keys)
		newBlock = TryAddToBlock(newBlock, txId, addedToBlock);

	return newBlock;
}

void Mempool::AddTxToMempool(const std::shared_ptr<Tx>& tx)
{
	std::scoped_lock lock(Mutex);

	const auto txId = tx->Id();
	if (Map.contains(txId))
	{
		LOG_INFO("Transaction {} already seen", txId);

		return;
	}

	try
	{
		tx->Validate(Tx::ValidateRequest());
	}
	catch (const TxValidationException& ex)
	{
		if (ex.ToOrphan != nullptr)
		{
			LOG_INFO("Transaction {} submitted as orphan", ex.ToOrphan->Id());

			OrphanedTxs.push_back(ex.ToOrphan);

			return;
		}
		LOG_ERROR("Transaction {} rejected", txId);

		return;
	}

	Map[txId] = tx;

	LOG_TRACE("Transaction {} added to mempool", txId);

	NetClient::SendMsgRandom(TxInfoMsg(tx));
}

bool Mempool::CheckBlockSize(const std::shared_ptr<Block>& block)
{
	return block->Serialize().GetSize() < NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES;
}

std::shared_ptr<Block> Mempool::TryAddToBlock(std::shared_ptr<Block>& block, const std::string& txId,
                                              std::set<std::string>& addedToBlock)
{
	std::scoped_lock lock(Mutex);

	if (addedToBlock.contains(txId))
		return block;

	if (!Map.contains(txId))
		return block;

	const auto& tx = Map[txId];

	for (const auto& txIn : tx->TxIns)
	{
		const auto& toSpend = txIn->ToSpend;

		if (UTXO::FindInMap(toSpend) != nullptr)
			continue;

		const auto& inMempool = Find_UTXO_InMempool(toSpend);
		if (inMempool == nullptr)
		{
			LOG_ERROR("Unable to find UTXO for {}", txIn->ToSpend->TxId);

			return nullptr;
		}

		block = TryAddToBlock(block, inMempool->TxOutPoint->TxId, addedToBlock);
		if (block == nullptr)
		{
			LOG_ERROR("Unable to add parent");

			return nullptr;
		}
	}

	auto newBlock = std::make_shared<Block>(*block);
	const auto& blockTxs = block->Txs;
	std::vector<std::shared_ptr<Tx>> txs;
	txs.insert(txs.end(), blockTxs.begin(), blockTxs.end());
	txs.push_back(tx);
	newBlock->Txs = txs;

	if (CheckBlockSize(newBlock))
	{
		LOG_TRACE("Added transaction {} to block {}", txId, block->Id());

		addedToBlock.insert(txId);

		return newBlock;
	}
	return block;
}
