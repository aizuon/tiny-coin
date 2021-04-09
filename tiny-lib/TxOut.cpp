#include "pch.hpp"

#include "TxOut.hpp"
#include "BinaryBuffer.hpp"

std::vector<uint8_t> TxOut::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(Value);
	buffer.Write(ToAddress);
	
	return buffer.GetBuffer();
}
