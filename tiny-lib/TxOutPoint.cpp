#include "pch.hpp"

#include "TxOutPoint.hpp"
#include "BinaryBuffer.hpp"

TxOutPoint::TxOutPoint(const std::string& txId, uint64_t txOutId)
	: TxId(txId), TxOutId(txOutId)
{
}

std::vector<uint8_t> TxOutPoint::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(TxId);
	buffer.Write(TxOutId);

	return buffer.GetBuffer();
}
