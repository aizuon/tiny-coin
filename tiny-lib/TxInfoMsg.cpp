#include "pch.hpp"

#include "TxInfoMsg.hpp"

TxInfoMsg::TxInfoMsg(const std::shared_ptr<::Tx>& tx)
	: Tx(tx)
{

}

void TxInfoMsg::Handle(const std::shared_ptr<NetClient::Connection>& con) const
{
	//TODO
}

BinaryBuffer TxInfoMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.WriteRaw(Tx->Serialize().GetBuffer());

	return buffer;
}

bool TxInfoMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	auto tx = std::make_shared<::Tx>();
	if (!tx->Deserialize(buffer))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

Opcode TxInfoMsg::GetOpcode() const
{
	return Opcode::TxInfoMsg;
}
