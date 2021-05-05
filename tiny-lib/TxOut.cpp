#include "pch.hpp"
#include "TxOut.hpp"

TxOut::TxOut(uint64_t value, const std::string& toAddress)
	: Value(value), ToAddress(toAddress)
{
}

BinaryBuffer TxOut::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(Value);
	buffer.Write(ToAddress);

	return buffer;
}

bool TxOut::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	if (!buffer.Read(Value))
	{
		*this = std::move(copy);

		return false;
	}
	if (!buffer.Read(ToAddress))
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
