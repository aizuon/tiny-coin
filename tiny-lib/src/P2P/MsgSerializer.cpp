#include "pch.hpp"
#include "P2P/MsgSerializer.hpp"

#include "BinaryBuffer.hpp"
#include "Crypto/SHA256.hpp"
#include "Tx/TxOut.hpp"
#include "Tx/TxOutPoint.hpp"

std::vector<uint8_t> MsgSerializer::BuildSpendMsg(std::shared_ptr<TxOutPoint> to_spend,
	const std::vector<uint8_t>& pub_key, int32_t sequence,
	const std::vector<std::shared_ptr<TxOut>>& tx_outs)
{
	BinaryBuffer spend_message;
	spend_message.WriteRaw(to_spend->Serialize().GetBuffer());
	spend_message.Write(sequence);
	spend_message.Write(pub_key);
	for (const auto& tx_out : tx_outs)
	{
		spend_message.WriteRaw(tx_out->Serialize().GetBuffer());
	}

	const auto buffer = spend_message.GetBuffer();

	auto hash = SHA256::DoubleHashBinary(buffer);

	return hash;
}
