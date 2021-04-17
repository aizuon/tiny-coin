#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "ISerializable.hpp"

class TxOutPoint : public ISerializable
{
public:
	TxOutPoint(const std::string& txId, int64_t txOutIdx);

	std::string TxId;
	int64_t TxOutIdx;

	std::vector<uint8_t> Serialize() const override;
};