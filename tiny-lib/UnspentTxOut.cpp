#include "pch.hpp"

#include "UnspentTxOut.hpp"
#include "BinaryBuffer.hpp"

UnspentTxOut::UnspentTxOut(uint64_t value, const std::string& toAddress, std::shared_ptr<TxOutPoint> txOut, bool isCoinbase, int32_t height)
	: Value(value), ToAddress(toAddress), TxOut(txOut), IsCoinbase(isCoinbase), Height(height)
{
}

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
