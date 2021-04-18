#include "pch.hpp"

#include "TxOutPoint.hpp"

TxOutPoint::TxOutPoint(const std::string& txId, int64_t txOutIdx)
	: TxId(txId), TxOutIdx(txOutIdx)
{

}

BinaryBuffer TxOutPoint::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(TxId);
	buffer.Write(TxOutIdx);

	return buffer;
}

bool TxOutPoint::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	if (!buffer.Read(TxId))
	{
		*this = std::move(copy);

		return false;
	}
	if (!buffer.Read(TxOutIdx))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}
