#include "pch.hpp"
#include "SendUTXOsMsg.hpp"

#include "MsgCache.hpp"
#include "NetClient.hpp"
#include "UnspentTxOut.hpp"

void SendUTXOsMsg::Handle(std::shared_ptr<Connection> con)
{
	MsgCache::SendUTXOsMsg = std::make_shared<SendUTXOsMsg>(*this);
}

BinaryBuffer SendUTXOsMsg::Serialize() const
{
	BinaryBuffer buffer;

	{
		std::scoped_lock lock(UTXO::Mutex);

		buffer.WriteSize(UTXO::Map.size());
		for (const auto& [key, value] : UTXO::Map)
		{
			buffer.WriteRaw(key->Serialize().GetBuffer());
			buffer.WriteRaw(value->Serialize().GetBuffer());
		}
	}

	return buffer;
}

bool SendUTXOsMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	uint32_t utxo_map_size = 0;
	if (!buffer.ReadSize(utxo_map_size))
	{
		*this = std::move(copy);

		return false;
	}
	if (!UTXO_Map.empty())
		UTXO_Map.clear();
	UTXO_Map.reserve(utxo_map_size);
	for (uint32_t i = 0; i < utxo_map_size; i++)
	{
		auto tx_out_point = std::make_shared<TxOutPoint>();
		if (!tx_out_point->Deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		const auto utxo = std::make_shared<UTXO>();
		if (!utxo->Deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		UTXO_Map[tx_out_point] = utxo;
	}

	return true;
}

Opcode SendUTXOsMsg::GetOpcode() const
{
	return Opcode::SendUTXOsMsg;
}
