#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "ISerializable.hpp"

class TxOutPoint : public ISerializable
{
public:
	TxOutPoint(const std::string& txId, int64_t txOutId)
		: TxId(txId), TxOutId(txOutId)
	{

	}

	std::string TxId;
	int64_t TxOutId;

	std::vector<uint8_t> Serialize() const override;
};