#pragma once
#include <cstdint>
#include <string>
#include <tuple>

#include "IDeserializable.hpp"
#include "ISerializable.hpp"

class TxOut : public ISerializable, public IDeserializable
{
public:
	TxOut() = default;
	TxOut(uint64_t value, const std::string& toAddress);

	uint64_t Value = 0;
	std::string ToAddress;

	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	bool operator==(const TxOut& obj) const;

private:
	auto tied() const
	{
		return std::tie(Value, ToAddress);
	}
};
