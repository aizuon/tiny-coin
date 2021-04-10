#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#include "TxOutPoint.hpp"
#include "ISerializable.hpp"

class UnspentTxOut : public ISerializable
{
public:
	UnspentTxOut(int64_t value, const std::string& toAddress, std::shared_ptr<TxOutPoint> txOut, bool isCoinbase, int64_t height);

	int64_t Value;
	std::string ToAddress;

	std::shared_ptr<TxOutPoint> TxOut;

	bool IsCoinbase;

	int64_t Height;

	std::vector<uint8_t> Serialize() const override;
};