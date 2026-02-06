#include "core/tx_out.hpp"

TxOut::TxOut(uint64_t value, std::string to_address)
	: value(value), to_address(std::move(to_address))
{}

BinaryBuffer TxOut::serialize() const
{
	BinaryBuffer buffer;

	buffer.write(value);
	buffer.write(to_address);

	return buffer;
}

bool TxOut::deserialize(BinaryBuffer& buffer)
{
	uint64_t new_value = 0;
	if (!buffer.read(new_value))
		return false;

	std::string new_to_address;
	if (!buffer.read(new_to_address))
		return false;

	value = new_value;
	to_address = std::move(new_to_address);

	return true;
}

bool TxOut::operator==(const TxOut& obj) const
{
	if (this == &obj)
	{
		return true;
	}

	return tied() == obj.tied();
}
