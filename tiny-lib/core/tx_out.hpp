#pragma once
#include <cstdint>
#include <string>
#include <tuple>

#include "util/i_deserializable.hpp"
#include "util/i_serializable.hpp"

class TxOut : public ISerializable, public IDeserializable
{
public:
	TxOut() = default;
	TxOut(uint64_t value, const std::string& to_address);

	uint64_t value = 0;
	std::string to_address;

	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	bool operator==(const TxOut& obj) const;

private:
	auto tied() const
	{
		return std::tie(value, to_address);
	}
};
