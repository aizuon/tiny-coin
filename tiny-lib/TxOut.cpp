#include "pch.hpp"

#include "TxOut.hpp"
#include "BinaryBuffer.hpp"

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
