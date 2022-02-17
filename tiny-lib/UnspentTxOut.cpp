#include "pch.hpp"
#include "UnspentTxOut.hpp"

#include <ranges>

#include "Log.hpp"

UnspentTxOut::UnspentTxOut(const std::shared_ptr<::TxOut>& txOut, const std::shared_ptr<::TxOutPoint>& txOutPoint,
                           bool isCoinbase, int64_t height)
	: TxOut(txOut), TxOutPoint(txOutPoint), IsCoinbase(isCoinbase), Height(height)
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

void UnspentTxOut::AddToMap(std::shared_ptr<::TxOut>& txOut, const std::string& txId, int64_t idx, bool isCoinbase,
                            int64_t height)
{
	std::scoped_lock lock(Mutex);

	auto txOutPoint = std::make_shared<::TxOutPoint>(txId, idx);

	const auto utxo = std::make_shared<UnspentTxOut>(txOut, txOutPoint, isCoinbase, height);

	LOG_TRACE("Adding TxOutPoint {} to UTXO map", utxo->TxOutPoint->TxId);

	Map[utxo->TxOutPoint] = utxo;
}

void UnspentTxOut::RemoveFromMap(const std::string& txId, int64_t idx)
{
	std::scoped_lock lock(Mutex);

	const auto map_it = std::ranges::find_if(Map,
	                                         [&txId, idx](
	                                         const std::pair<
		                                         std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>>& p)
	                                         {
		                                         const auto& [txOutPoint, utxo] = p;
		                                         return txOutPoint->TxId == txId && txOutPoint->TxOutIdx == idx;
	                                         });
	if (map_it != Map.end())
		Map.erase(map_it);
}

std::shared_ptr<UnspentTxOut> UnspentTxOut::FindInList(const std::shared_ptr<TxIn>& txIn,
                                                       const std::vector<std::shared_ptr<Tx>>& txs)
{
	for (const auto& tx : txs)
	{
		const auto& toSpend = txIn->ToSpend;

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

std::shared_ptr<UnspentTxOut> UnspentTxOut::FindInMap(const std::shared_ptr<::TxOutPoint>& toSpend)
{
	std::scoped_lock lock(Mutex);

	const auto map_it = std::ranges::find_if(Map,
	                                         [&toSpend](
	                                         const std::pair<
		                                         std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>>&
	                                         p)
	                                         {
		                                         const auto& [txOutPoint, utxo] = p;
		                                         return *txOutPoint == *toSpend;
	                                         });
	if (map_it != Map.end())
	{
		return map_it->second;
	}

	return nullptr;
}

std::shared_ptr<TxOut> UnspentTxOut::FindTxOutInBlock(const std::shared_ptr<Block>& block,
                                                      const std::shared_ptr<TxIn>& txIn)
{
	for (const auto& tx : block->Txs)
	{
		if (tx->Id() == txIn->ToSpend->TxId)
		{
			return tx->TxOuts[txIn->ToSpend->TxOutIdx];
		}
	}

	return nullptr;
}

std::shared_ptr<TxOut> UnspentTxOut::FindTxOutInMap(const std::shared_ptr<TxIn>& txIn)
{
	std::scoped_lock lock(Mutex);

	for (const auto& utxo : Map | std::ranges::views::values)
	{
		if (txIn->ToSpend->TxId == utxo->TxOutPoint->TxId && txIn->ToSpend->TxOutIdx == utxo->TxOutPoint->TxOutIdx)
		{
			return utxo->TxOut;
		}
	}

	return nullptr;
}

std::shared_ptr<TxOut> UnspentTxOut::FindTxOutInMapOrBlock(const std::shared_ptr<Block>& block,
                                                           const std::shared_ptr<TxIn>& txIn)
{
	auto utxo = FindTxOutInMap(txIn);
	if (utxo != nullptr)
		return utxo;

	return FindTxOutInBlock(block, txIn);
}

bool UnspentTxOut::operator==(const UnspentTxOut& obj) const
{
	if (this == &obj)
	{
		return true;
	}

	return tied() == obj.tied() && *TxOut == *obj.TxOut && *TxOutPoint == *obj.TxOutPoint;
}
