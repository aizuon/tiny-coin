#include "pch.hpp"

#include "TxOutPoint.hpp"
#include "BinaryBuffer.hpp"

TxOutPoint::TxOutPoint(const std::string& txId, int64_t txOutIdx)
	: TxId(txId), TxOutIdx(txOutIdx)
{
}

std::vector<uint8_t> TxOutPoint::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(TxId);
	buffer.Write(TxOutIdx);

	return buffer.GetBuffer();
}
