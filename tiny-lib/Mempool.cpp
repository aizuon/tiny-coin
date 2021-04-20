#include "pch.hpp"

#include <ranges>

#include "Mempool.hpp"
#include "BinaryBuffer.hpp"
#include "Exceptions.hpp"
#include "Log.hpp"
#include "NetParams.hpp"
#include "TxInfoMsg.hpp"
#include "NetClient.hpp"

std::unordered_map<std::string, std::shared_ptr<Tx>> Mempool::Map;

std::vector<std::shared_ptr<Tx>> Mempool::OrphanedTxs;

std::shared_ptr<UnspentTxOut> Mempool::Find_UTXO_InMempool(const std::shared_ptr<TxOutPoint>& txOutPoint)
{
	if (!Map.contains(txOutPoint->TxId))
		return nullptr;

	const auto& tx = Map[txOutPoint->TxId];
	if (tx->TxOuts.size() - 1 < txOutPoint->TxOutIdx)
	{
		LOG_TRACE("Couldn't find UTXO in mempool for {}", txOutPoint->TxId);

		return nullptr;
	}

	const auto& txOut = tx->TxOuts[txOutPoint->TxOutIdx];

	return std::make_shared<UnspentTxOut>(txOut, txOutPoint, false, -1);
}

std::shared_ptr<Block> Mempool::SelectFromMempool(const std::shared_ptr<Block>& block)
{
	auto newBlock = std::make_shared<Block>(*block);

	std::set<std::string> addedToBlock;
	for (const auto& [txId, tx] : Map)
		newBlock = TryAddToBlock(newBlock, txId, addedToBlock);

	return newBlock;
}

void Mempool::AddTxToMempool(const std::shared_ptr<Tx>& tx)
{
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
		else
		{
			LOG_ERROR("Transaction {} rejected", txId);

			return;
		}
	}

	Map[txId] = tx;

	LOG_INFO("Transaction {} added to mempool", txId);

	NetClient::BroadcastMsgAsync(TxInfoMsg(tx));
}

bool Mempool::CheckBlockSize(const std::shared_ptr<Block>& block)
{
	return block->Serialize().GetSize() < NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES;
}

std::shared_ptr<Block> Mempool::TryAddToBlock(std::shared_ptr<Block>& block, const std::string& txId, std::set<std::string>& addedToBlock)
{
	if (addedToBlock.contains(txId))
		return block;

	if (!Map.contains(txId))
		return block;

	const auto& tx = Map[txId];

	for (const auto& txIn : tx->TxIns)
	{
		const auto& toSpend = txIn->ToSpend;

		auto map_it = std::find_if(UnspentTxOut::Map.begin(), UnspentTxOut::Map.end(),
			[&toSpend](const std::pair<std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>>& p)
			{
				auto& [txOutPoint, utxo] = p;
				return *txOutPoint == *toSpend;
			});
		if (map_it != UnspentTxOut::Map.end())
			continue;

		const auto& inMempool = Find_UTXO_InMempool(toSpend);
		if (inMempool == nullptr)
		{
			LOG_TRACE("Couldn't find UTXO for {}", txIn->ToSpend->TxId);

			return nullptr;
		}

		block = TryAddToBlock(block, inMempool->TxOutPoint->TxId, addedToBlock);
		if (block == nullptr)
		{
			LOG_TRACE("Couldn't add parent");

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
	else
	{
		return block;
	}
}
