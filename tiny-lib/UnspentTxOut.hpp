#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <memory>

#include "ISerializable.hpp"
#include "TxOut.hpp"
#include "TxOutPoint.hpp"
#include "Tx.hpp"
#include "TxIn.hpp"

class UnspentTxOut : public ISerializable
{
public:
	UnspentTxOut(std::shared_ptr<TxOut> txOut, std::shared_ptr<TxOutPoint> txOutPoint, bool isCoinbase, int32_t height);

	std::shared_ptr<TxOut> TxOut;

	std::shared_ptr<TxOutPoint> TxOutPoint;

	bool IsCoinbase;

	int32_t Height;

	std::vector<uint8_t> Serialize() const override;

	static std::map<std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>> Set;
	static void AddToSet(std::shared_ptr<::TxOut> txOut, const std::string& txId, uint64_t idx, bool isCoinbase, int32_t height);
	static void RemoveFromSet(const std::string& txId, uint64_t idx);
	static std::shared_ptr<UnspentTxOut> FindInList(const std::shared_ptr<TxIn>& txIn, const std::vector<std::shared_ptr<Tx>>& txs);
};