#include "pch.hpp"

#include "TxOutPoint.hpp"
#include "BinaryBuffer.hpp"

std::vector<uint8_t> TxOutPoint::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(TxId);
	buffer.Write(TxOutId);

	return buffer.GetBuffer();
}
