#include "pch.hpp"
#include "UnspentTxOut.hpp"

#include <ranges>

#include "Log.hpp"

UnspentTxOut::UnspentTxOut(std::shared_ptr<::TxOut> tx_out, std::shared_ptr<::TxOutPoint> tx_out_point,
                           bool is_coinbase, int64_t height)
	: TxOut(tx_out), TxOutPoint(tx_out_point), IsCoinbase(is_coinbase), Height(height)
{
}

BinaryBuffer UnspentTxOut::Serialize() const
{
	BinaryBuffer buffer;

	buffer.WriteRaw(TxOut->Serialize().GetBuffer());
	buffer.WriteRaw(TxOutPoint->Serialize().GetBuffer());
	buffer.Write(IsCoinbase);
	buffer.Write(Height);

	return buffer;
}

bool UnspentTxOut::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	TxOut = std::make_shared<::TxOut>();
	if (!TxOut->Deserialize(buffer))
	{
		*this = std::move(copy);

		return false;
	}

	TxOutPoint = std::make_shared<::TxOutPoint>();
	if (!TxOutPoint->Deserialize(buffer))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.Read(IsCoinbase))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.Read(Height))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

std::unordered_map<std::shared_ptr<TxOutPoint>, std::shared_ptr<UnspentTxOut>> UnspentTxOut::Map;

std::recursive_mutex UnspentTxOut::Mutex;

void UnspentTxOut::AddToMap(std::shared_ptr<::TxOut> tx_out, const std::string& tx_id, int64_t idx, bool is_coinbase,
                            int64_t height)
{
	std::scoped_lock lock(Mutex);

	auto txOutPoint = std::make_shared<::TxOutPoint>(tx_id, idx);

	const auto utxo = std::make_shared<UnspentTxOut>(tx_out, txOutPoint, is_coinbase, height);

	LOG_TRACE("Adding TxOutPoint {} to UTXO map", utxo->TxOutPoint->TxId);

	Map[utxo->TxOutPoint] = utxo;
}

void UnspentTxOut::RemoveFromMap(const std::string& tx_id, int64_t idx)
{
	std::scoped_lock lock(Mutex);

	const auto map_it = std::ranges::find_if(Map,
	                                         [&tx_id, idx](
	                                         const std::pair<
		                                         std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>>& p)
	                                         {
		                                         const auto& [txOutPoint, utxo] = p;
		                                         return txOutPoint->TxId == tx_id && txOutPoint->TxOutIdx == idx;
	                                         });
	if (map_it != Map.end())
		Map.erase(map_it);
}

std::shared_ptr<UnspentTxOut> UnspentTxOut::FindInList(std::shared_ptr<TxIn> tx_in,
                                                       const std::vector<std::shared_ptr<Tx>>& txs)
{
	for (const auto& tx : txs)
	{
		const auto& toSpend = tx_in->ToSpend;

		if (tx->Id() == toSpend->TxId)
		{
			if (tx->TxOuts.size() - 1 < toSpend->TxOutIdx)
				return nullptr;

			auto& matchingTxOut = tx->TxOuts[toSpend->TxOutIdx];
			auto txOutPoint = std::make_shared<::TxOutPoint>(toSpend->TxId, toSpend->TxOutIdx);
			return std::make_shared<UnspentTxOut>(matchingTxOut, txOutPoint, false, -1);
		}
	}

	return nullptr;
}

std::shared_ptr<UnspentTxOut> UnspentTxOut::FindInMap(std::shared_ptr<::TxOutPoint> to_spend)
{
	std::scoped_lock lock(Mutex);

	const auto map_it = std::ranges::find_if(Map,
	                                         [&to_spend](
	                                         const std::pair<
		                                         std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>>&
	                                         p)
	                                         {
		                                         const auto& [txOutPoint, utxo] = p;
		                                         return *txOutPoint == *to_spend;
	                                         });
	if (map_it != Map.end())
	{
		return map_it->second;
	}

	return nullptr;
}

std::shared_ptr<TxOut> UnspentTxOut::FindTxOutInBlock(std::shared_ptr<Block> block,
                                                      std::shared_ptr<TxIn> tx_in)
{
	for (const auto& tx : block->Txs)
	{
		if (tx->Id() == tx_in->ToSpend->TxId)
		{
			return tx->TxOuts[tx_in->ToSpend->TxOutIdx];
		}
	}

	return nullptr;
}

std::shared_ptr<TxOut> UnspentTxOut::FindTxOutInMap(std::shared_ptr<TxIn> tx_in)
{
	std::scoped_lock lock(Mutex);

	for (const auto& utxo : Map | std::ranges::views::values)
	{
		if (tx_in->ToSpend->TxId == utxo->TxOutPoint->TxId && tx_in->ToSpend->TxOutIdx == utxo->TxOutPoint->TxOutIdx)
		{
			return utxo->TxOut;
		}
	}

	return nullptr;
}

std::shared_ptr<TxOut> UnspentTxOut::FindTxOutInMapOrBlock(std::shared_ptr<Block> block,
                                                           std::shared_ptr<TxIn> tx_in)
{
	auto utxo = FindTxOutInMap(tx_in);
	if (utxo != nullptr)
		return utxo;

	return FindTxOutInBlock(block, tx_in);
}

bool UnspentTxOut::operator==(const UnspentTxOut& obj) const
{
	if (this == &obj)
	{
		return true;
	}

	return tied() == obj.tied() && *TxOut == *obj.TxOut && *TxOutPoint == *obj.TxOutPoint;
}
