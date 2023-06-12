#include "pch.hpp"
#include "P2P/GetUTXOsMsg.hpp"

#include "P2P/NetClient.hpp"
#include "P2P/SendUTXOsMsg.hpp"

void GetUTXOsMsg::Handle(std::shared_ptr<Connection> con)
{
	NetClient::SendMsg(con, SendUTXOsMsg());
}

BinaryBuffer GetUTXOsMsg::Serialize() const
{
	BinaryBuffer buffer;

	return buffer;
}

bool GetUTXOsMsg::Deserialize(BinaryBuffer& buffer)
{
	return true;
}

Opcode GetUTXOsMsg::GetOpcode() const
{
	return Opcode::GetUTXOsMsg;
}
