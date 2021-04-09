#include "pch.hpp"

#include "TxIn.hpp"
#include "BinaryBuffer.hpp"

std::vector<uint8_t> TxIn::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(ToSpend->Serialize());
	buffer.Write(UnlockSig);
	buffer.Write(UnlockPk);
	buffer.Write(Sequence);

	return buffer.GetBuffer();
}
