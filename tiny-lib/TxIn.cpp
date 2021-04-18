#include "pch.hpp"

#include "TxIn.hpp"

TxIn::TxIn(std::shared_ptr<TxOutPoint> toSpend, const std::vector<uint8_t>& unlockSig, const std::vector<uint8_t>& unlockPk, int32_t sequence)
	: ToSpend(toSpend), UnlockSig(unlockSig), UnlockPubKey(unlockPk), Sequence(sequence)
{

}

BinaryBuffer TxIn::Serialize() const
{
	BinaryBuffer buffer;

	buffer.WriteRaw(ToSpend->Serialize().GetBuffer());
	buffer.Write(UnlockSig);
	buffer.Write(UnlockPubKey);
	buffer.Write(Sequence);

	return buffer;
}

bool TxIn::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	ToSpend = std::make_shared<TxOutPoint>();
	if (!ToSpend->Deserialize(buffer))
	{
		*this = std::move(copy);

		return false;
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
