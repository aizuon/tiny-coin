#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "IDeserializable.hpp"
#include "ISerializable.hpp"
#include "Tx.hpp"
#include "TxIn.hpp"
#include "TxOut.hpp"
#include "TxOutPoint.hpp"

class UnspentTxOut : public ISerializable, public IDeserializable
{
public:
	UnspentTxOut() = default;
	UnspentTxOut(const std::shared_ptr<TxOut>& txOut, const std::shared_ptr<TxOutPoint>& txOutPoint, bool isCoinbase,
	             int64_t height);

	std::shared_ptr<TxOut> TxOut;

	std::shared_ptr<TxOutPoint> TxOutPoint;

	bool IsCoinbase = false;

	int64_t Height = -1;

	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	//TODO: map mutex?
	static std::unordered_map<std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>> Map;

	static void AddToMap(std::shared_ptr<::TxOut> txOut, const std::string& txId, int64_t idx, bool isCoinbase,
	                     int64_t height);
	static void RemoveFromMap(const std::string& txId, int64_t idx);

	static std::shared_ptr<UnspentTxOut> FindInList(const std::shared_ptr<TxIn>& txIn,
	                                                const std::vector<std::shared_ptr<Tx>>& txs);

	bool operator==(const UnspentTxOut& obj) const;

private:
	auto tied() const
	{
		return std::tie(IsCoinbase, Height);
	}
};

using UTXO = UnspentTxOut;
