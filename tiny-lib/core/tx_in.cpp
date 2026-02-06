#include "core/tx_in.hpp"

TxIn::TxIn(std::shared_ptr<TxOutPoint> to_spend, const std::vector<uint8_t>& unlock_sig,
	const std::vector<uint8_t>& unlock_pub_key, int32_t sequence)
	: to_spend(to_spend), unlock_sig(unlock_sig), unlock_pub_key(unlock_pub_key), sequence(sequence)
{}

BinaryBuffer TxIn::serialize() const
{
	BinaryBuffer buffer;

	const bool has_to_spend = to_spend != nullptr;
	buffer.write(has_to_spend);
	if (has_to_spend)
		buffer.write_raw(to_spend->serialize().get_buffer());
	buffer.write(unlock_sig);
	buffer.write(unlock_pub_key);
	buffer.write(sequence);

	return buffer;
}

bool TxIn::deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	bool has_to_spend = false;
	if (!buffer.read(has_to_spend))
	{
		*this = std::move(copy);

		return false;
	}
	if (has_to_spend)
	{
		to_spend = std::make_shared<TxOutPoint>();
		if (!to_spend->deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
	}

	if (!buffer.read(unlock_sig))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.read(unlock_pub_key))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.read(sequence))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

bool TxIn::operator==(const TxIn& obj) const
{
	if (this == &obj)
		return true;

	if (tied() != obj.tied())
		return false;

	if (to_spend == nullptr && obj.to_spend == nullptr)
		return true;
	if (to_spend == nullptr || obj.to_spend == nullptr)
		return false;

	return *to_spend == *obj.to_spend;
}
