#include "pch.hpp"

#include <algorithm>

#include "SendUTXOsMsg.hpp"
#include "MsgCache.hpp"
#include "NetClient.hpp"
#include "UnspentTxOut.hpp"

void SendUTXOsMsg::Handle(std::shared_ptr<Connection>& con)
{
	MsgCache::SendUTXOsMsg = std::make_shared<SendUTXOsMsg>(*this);
}

BinaryBuffer SendUTXOsMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(UTXO::Map.size());
	for (const auto& [key, value] : UTXO::Map)
	{
		buffer.WriteRaw(key->Serialize().GetBuffer());
		buffer.WriteRaw(value->Serialize().GetBuffer());
	}

	return buffer;
}

bool SendUTXOsMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	size_t utxoMapSize = 0;
	if (!buffer.Read(utxoMapSize))
	{
		*this = std::move(copy);

		return false;
	}
	UTXO_Map = std::unordered_map<std::shared_ptr<TxOutPoint>, std::shared_ptr<UnspentTxOut>>();
	UTXO_Map.reserve(utxoMapSize);
	for (size_t i = 0; i < utxoMapSize; i++)
	{
		auto txOutPoint = std::make_shared<TxOutPoint>();
		if (!txOutPoint->Deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		auto utxo = std::make_shared<UnspentTxOut>();
		if (!utxo->Deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		UTXO_Map[txOutPoint] = utxo;
	}

	return true;
}

Opcode SendUTXOsMsg::GetOpcode() const
{
	return Opcode::SendUTXOsMsg;
}
