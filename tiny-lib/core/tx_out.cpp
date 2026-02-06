#include "core/tx_out.hpp"

TxOut::TxOut(uint64_t value, const std::string& to_address)
	: value(value), to_address(to_address)
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
	auto copy = *this;

	if (!buffer.read(value))
	{
		*this = std::move(copy);

		return false;
	}
	if (!buffer.read(to_address))
	{
		*this = std::move(copy);

		return false;
	}

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
