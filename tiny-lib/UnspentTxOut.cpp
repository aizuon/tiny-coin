#include "pch.hpp"

#include <algorithm>

#include "UnspentTxOut.hpp"
#include "BinaryBuffer.hpp"

UnspentTxOut::UnspentTxOut(std::shared_ptr<::TxOut> txOut, std::shared_ptr<::TxOutPoint> txOutPoint, bool isCoinbase, int64_t height)
	: TxOut(txOut), TxOutPoint(txOutPoint), IsCoinbase(isCoinbase), Height(height)
{
}

std::vector<uint8_t> UnspentTxOut::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(this->TxOut->Serialize());
	buffer.Write(this->TxOutPoint->Serialize());
	buffer.Write(IsCoinbase);
	buffer.Write(Height);

	return buffer.GetBuffer();
}

void UnspentTxOut::AddToSet(std::shared_ptr<::TxOut> txOut, const std::string& txId, int64_t idx, bool isCoinbase, int64_t height)
{
	auto txOutPoint = std::make_shared<::TxOutPoint>(txId, idx);

	auto utxo = std::make_shared<UnspentTxOut>(txOut, txOutPoint, isCoinbase, height);

	Set[utxo->TxOutPoint] = utxo;
}

void UnspentTxOut::RemoveFromSet(const std::string& txId, int64_t idx)
{
	auto set_it = std::find_if(Set.begin(), Set.end(),
		[&txId, idx](const std::pair<std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>>& p)
		{
			auto& [txOutPoint, utxo] = p;
			return txOutPoint->TxId == txId && txOutPoint->TxOutId == idx;
		});
	if (set_it != Set.end())
		Set.erase(set_it);
}

std::shared_ptr<UnspentTxOut> UnspentTxOut::FindInList(const std::shared_ptr<TxIn>& txIn, const std::vector<std::shared_ptr<Tx>>& txs)
{
	for (const auto& tx : txs)
	{
		const auto& toSpend = txIn->ToSpend;

		if (tx->Id() == toSpend->TxId)
		{
			if (tx->TxOuts.size() - 1 < toSpend->TxOutId)
				return nullptr;

			auto matchingTxOut = tx->TxOuts[toSpend->TxOutId];
			auto txOutPoint = std::make_shared<::TxOutPoint>(toSpend->TxId, toSpend->TxOutId);
			return std::make_shared<UnspentTxOut>(matchingTxOut, txOutPoint, false, -1);
		}
	}

	return nullptr;
}
