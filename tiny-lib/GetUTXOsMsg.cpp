#include "pch.hpp"

#include <algorithm>

#include "GetUTXOsMsg.hpp"

GetUTXOsMsg::GetUTXOsMsg(const std::unordered_map<std::shared_ptr<TxOutPoint>, std::shared_ptr<UnspentTxOut>>& utxoMap)
	: UTXO_Map(utxoMap)
{

}

void GetUTXOsMsg::Handle(const std::shared_ptr<NetClient::Connection>& con) const
{
	//TODO
}

BinaryBuffer GetUTXOsMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(UTXO_Map.size());
	for (const auto& [key, value] : UTXO_Map)
	{
		buffer.WriteRaw(key->Serialize().GetBuffer());
		buffer.WriteRaw(value->Serialize().GetBuffer());
	}

	return buffer;
}

bool GetUTXOsMsg::Deserialize(BinaryBuffer& buffer)
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
		auto map_it = std::find_if(UTXO_Map.begin(), UTXO_Map.end(),
			[&txOutPoint](const std::pair<std::shared_ptr<TxOutPoint>, std::shared_ptr<UnspentTxOut>>& p)
			{
				auto& [txOutPoint2, utxo2] = p;
				return txOutPoint->TxId == txOutPoint2->TxId && txOutPoint->TxOutIdx == txOutPoint2->TxOutIdx;
			});
		if (map_it == UTXO_Map.end())
		{
			UTXO_Map[txOutPoint] = utxo;
		}
	}

	return true;
}

Opcode GetUTXOsMsg::GetOpcode() const
{
	return Opcode::GetUTXOsMsg;
}
