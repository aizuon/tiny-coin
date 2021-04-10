#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "ISerializable.hpp"

class TxOut : public ISerializable
{
public:
	TxOut(uint64_t value, const std::string& toAddress);

	uint64_t Value;

	std::string ToAddress;

	std::vector<uint8_t> Serialize() const override;
};