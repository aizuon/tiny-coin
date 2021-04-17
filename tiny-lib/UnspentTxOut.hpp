#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include "ISerializable.hpp"

class Tx;
class TxIn;
class TxOut;
class TxOutPoint;

class UnspentTxOut : public ISerializable
{
public:
	UnspentTxOut(std::shared_ptr<::TxOut> txOut, std::shared_ptr<::TxOutPoint> txOutPoint, bool isCoinbase, int64_t height);

	std::shared_ptr<::TxOut> TxOut;

	std::shared_ptr<::TxOutPoint> TxOutPoint;

	bool IsCoinbase;

	int64_t Height;

	std::vector<uint8_t> Serialize() const override;

	static std::unordered_map<std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>> Map;

	static void AddToMap(std::shared_ptr<::TxOut> txOut, const std::string& txId, int64_t idx, bool isCoinbase, int64_t height);
	static void RemoveFromMap(const std::string& txId, int64_t idx);

	static std::shared_ptr<UnspentTxOut> FindInList(const std::shared_ptr<TxIn>& txIn, const std::vector<std::shared_ptr<Tx>>& txs);
};