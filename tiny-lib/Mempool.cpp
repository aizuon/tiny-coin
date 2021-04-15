#include "pch.hpp"

#include <ranges>

#include "Mempool.hpp"
#include "NetParams.hpp"
#include "TxOutPoint.hpp"
#include "UnspentTxOut.hpp"

std::map<std::string, std::shared_ptr<Tx>> Mempool::Map;

std::vector<std::shared_ptr<Tx>> Mempool::OrphanedTxs;

std::shared_ptr<UnspentTxOut> Mempool::Find_UTXO_InMempool(const std::shared_ptr<TxOutPoint>& txOutPoint)
{
	if (!Map.contains(txOutPoint->TxId))
		return nullptr;

	const auto& tx = Map[txOutPoint->TxId];
	if (tx->TxOuts.size() - 1 < txOutPoint->TxOutId)
		return nullptr;

	const auto& txOut = tx->TxOuts[txOutPoint->TxOutId];

	return std::make_shared<UnspentTxOut>(txOut, txOutPoint, false, -1);
}

std::shared_ptr<Block> Mempool::SelectFromMempool(std::shared_ptr<Block>& block)
{
	std::set<std::string> addedToBlock;

	for (const auto& [txId, tx] : Map)
		block = TryAddToBlock(block, txId, addedToBlock);

	return block;
}

bool Mempool::CheckBlockSize(const std::shared_ptr<Block>& block)
{
	return block->Serialize().size() < NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES;
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
				return txOutPoint->TxId == toSpend->TxId && txOutPoint->TxOutId == toSpend->TxOutId;
			});
		if (map_it != UnspentTxOut::Map.end())
			continue;

		const auto& inMempool = Find_UTXO_InMempool(toSpend);
		if (inMempool == nullptr)
			return nullptr;

		block = TryAddToBlock(block, inMempool->TxOutPoint->TxId, addedToBlock);
		if (block == nullptr)
			return nullptr;
	}

	auto newBlock = std::make_shared<Block>(*block);
	std::vector<std::shared_ptr<Tx>> txs(block->Txs.size() + 1);
	txs.insert(txs.end(), block->Txs.begin(), block->Txs.end());
	txs.push_back(tx);
	newBlock->Txs = txs;

	if (CheckBlockSize(newBlock))
	{
		addedToBlock.insert(txId);

		return newBlock;
	}
	else
	{
		return block;
	}
}
