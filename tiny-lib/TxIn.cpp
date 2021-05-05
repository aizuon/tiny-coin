#include "pch.hpp"

#include "TxIn.hpp"

TxIn::TxIn(const std::shared_ptr<TxOutPoint>& toSpend, const std::vector<uint8_t>& unlockSig,
           const std::vector<uint8_t>& unlockPubKey, int32_t sequence)
	: ToSpend(toSpend), UnlockSig(unlockSig), UnlockPubKey(unlockPubKey), Sequence(sequence)
{
}

BinaryBuffer TxIn::Serialize() const
{
	BinaryBuffer buffer;

	const bool has_toSpend = ToSpend != nullptr;
	buffer.Write(has_toSpend);
	if (has_toSpend)
		buffer.WriteRaw(ToSpend->Serialize().GetBuffer());
	buffer.Write(UnlockSig);
	buffer.Write(UnlockPubKey);
	buffer.Write(Sequence);

	return buffer;
}

bool TxIn::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	bool has_toSpend = false;
	if (!buffer.Read(has_toSpend))
	{
		*this = std::move(copy);

		return false;
	}
	if (has_toSpend)
	{
		ToSpend = std::make_shared<TxOutPoint>();
		if (!ToSpend->Deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
	}

	if (!buffer.Read(UnlockSig))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.Read(UnlockPubKey))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.Read(Sequence))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

bool TxIn::operator==(const TxIn& obj) const
{
	if (this == &obj)
	{
		return true;
	}

	return tied() == obj.tied() && *ToSpend == *obj.ToSpend;
}
