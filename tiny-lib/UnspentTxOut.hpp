#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include "ISerializable.hpp"
#include "IDeserializable.hpp"

#include "Tx.hpp"
#include "TxIn.hpp"
#include "TxOut.hpp"
#include "TxOutPoint.hpp"

class UnspentTxOut : public ISerializable, public IDeserializable
{
public:
	UnspentTxOut() = default;
	UnspentTxOut(std::shared_ptr<::TxOut> txOut, std::shared_ptr<::TxOutPoint> txOutPoint, bool isCoinbase, int64_t height);

	std::shared_ptr<::TxOut> TxOut;

	std::shared_ptr<::TxOutPoint> TxOutPoint;

	bool IsCoinbase = false;

	int64_t Height = -1;

	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	static std::unordered_map<std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>> Map;

	static void AddToMap(std::shared_ptr<::TxOut> txOut, const std::string& txId, int64_t idx, bool isCoinbase, int64_t height);
	static void RemoveFromMap(const std::string& txId, int64_t idx);

	static std::shared_ptr<UnspentTxOut> FindInList(const std::shared_ptr<TxIn>& txIn, const std::vector<std::shared_ptr<Tx>>& txs);
};

typedef UnspentTxOut UTXO;