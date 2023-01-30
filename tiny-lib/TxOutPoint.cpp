#include "pch.hpp"
#include "TxOutPoint.hpp"

TxOutPoint::TxOutPoint(const std::string& tx_id, int64_t tx_out_idx)
	: TxId(tx_id), TxOutIdx(tx_out_idx)
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

bool TxOutPoint::operator==(const TxOutPoint& obj) const
{
	if (this == &obj)
	{
		return true;
	}

	return tied() == obj.tied();
}
