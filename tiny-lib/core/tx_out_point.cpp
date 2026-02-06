#include "core/tx_out_point.hpp"

TxOutPoint::TxOutPoint(std::string tx_id, int64_t tx_out_idx)
	: tx_id(std::move(tx_id)), tx_out_idx(tx_out_idx)
{}

BinaryBuffer TxOutPoint::serialize() const
{
	BinaryBuffer buffer;

	buffer.write(tx_id);
	buffer.write(tx_out_idx);

	return buffer;
}

bool TxOutPoint::deserialize(BinaryBuffer& buffer)
{
	std::string new_tx_id;
	if (!buffer.read(new_tx_id))
		return false;

	int64_t new_tx_out_idx = -1;
	if (!buffer.read(new_tx_out_idx))
		return false;

	tx_id = std::move(new_tx_id);
	tx_out_idx = new_tx_out_idx;

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
