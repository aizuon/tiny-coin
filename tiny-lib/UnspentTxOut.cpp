#include "pch.hpp"

#include "UnspentTxOut.hpp"
#include "BinaryBuffer.hpp"

std::vector<uint8_t> UnspentTxOut::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(Value);
	buffer.Write(ToAddress);
	buffer.Write(TxOut->Serialize());
	buffer.Write(IsCoinbase);
	buffer.Write(Height);

	return buffer.GetBuffer();
}
