#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "ISerializable.hpp"

class TxOutPoint : public ISerializable
{
public:
	TxOutPoint(const std::string& txId, uint64_t txOutId);

	std::string TxId;
	uint64_t TxOutId;

	std::vector<uint8_t> Serialize() const override;
};