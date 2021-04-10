#include "pch.hpp"

#include "TxOut.hpp"
#include "BinaryBuffer.hpp"

TxOut::TxOut(uint64_t value, const std::string& toAddress)
	: Value(value), ToAddress(toAddress)
{
}

std::vector<uint8_t> TxOut::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(Value);
	buffer.Write(ToAddress);
	
	return buffer.GetBuffer();
}
