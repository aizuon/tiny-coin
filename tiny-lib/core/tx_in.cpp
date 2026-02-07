#include "core/tx_in.hpp"

TxIn::TxIn(std::shared_ptr<TxOutPoint> to_spend, std::vector<uint8_t> unlock_sig,
	std::vector<uint8_t> unlock_pub_key, int32_t sequence)
	: to_spend(std::move(to_spend)), unlock_sig(std::move(unlock_sig)), unlock_pub_key(std::move(unlock_pub_key)), sequence(sequence)
{}

bool TxIn::has_relative_locktime() const
{
	return (static_cast<uint32_t>(sequence) & SEQUENCE_LOCKTIME_DISABLE_FLAG) == 0;
}

bool TxIn::is_time_based_locktime() const
{
	return (static_cast<uint32_t>(sequence) & SEQUENCE_LOCKTIME_TYPE_FLAG) != 0;
}

uint32_t TxIn::relative_locktime_value() const
{
	return static_cast<uint32_t>(sequence) & SEQUENCE_LOCKTIME_MASK;
}

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
	bool has_to_spend = false;
	if (!buffer.read(has_to_spend))
		return false;

	std::shared_ptr<TxOutPoint> new_to_spend;
	if (has_to_spend)
	{
		new_to_spend = std::make_shared<TxOutPoint>();
		if (!new_to_spend->deserialize(buffer))
			return false;
	}

	std::vector<uint8_t> new_unlock_sig;
	if (!buffer.read(new_unlock_sig))
		return false;

	std::vector<uint8_t> new_unlock_pub_key;
	if (!buffer.read(new_unlock_pub_key))
		return false;

	int32_t new_sequence = -1;
	if (!buffer.read(new_sequence))
		return false;

	to_spend = std::move(new_to_spend);
	unlock_sig = std::move(new_unlock_sig);
	unlock_pub_key = std::move(new_unlock_pub_key);
	sequence = new_sequence;

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
