#include "pch.hpp"

#include "TxIn.hpp"
#include "BinaryBuffer.hpp"

TxIn::TxIn(std::shared_ptr<TxOutPoint> toSpend, const std::vector<uint8_t>& unlockSig, const std::vector<uint8_t>& unlockPk, int32_t sequence)
	: ToSpend(toSpend), UnlockSig(unlockSig), UnlockPk(unlockPk), Sequence(sequence)
{
}

std::vector<uint8_t> TxIn::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(ToSpend->Serialize());
	buffer.Write(UnlockSig);
	buffer.Write(UnlockPk);
	buffer.Write(Sequence);

	return buffer.GetBuffer();
}