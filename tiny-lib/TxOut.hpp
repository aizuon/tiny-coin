#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "ISerializable.hpp"
#include "IDeserializable.hpp"

class TxOut : public ISerializable, public IDeserializable
{
public:
	TxOut() = default;
	TxOut(uint64_t value, const std::string& toAddress);

	uint64_t Value = 0;
	std::string ToAddress;

	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;
};