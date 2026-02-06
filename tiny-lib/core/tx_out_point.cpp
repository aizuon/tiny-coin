#include "core/tx_out_point.hpp"

TxOutPoint::TxOutPoint(const std::string& tx_id, int64_t tx_out_idx)
	: tx_id(tx_id), tx_out_idx(tx_out_idx)
{
}

BinaryBuffer TxOutPoint::serialize() const
{
	BinaryBuffer buffer;

	buffer.write(tx_id);
	buffer.write(tx_out_idx);

	return buffer;
}

bool TxOutPoint::deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	if (!buffer.read(tx_id))
	{
		*this = std::move(copy);

		return false;
	}
	if (!buffer.read(tx_out_idx))
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
