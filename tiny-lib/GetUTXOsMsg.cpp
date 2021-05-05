#include "pch.hpp"
#include "GetUTXOsMsg.hpp"

#include "NetClient.hpp"
#include "SendUTXOsMsg.hpp"

void GetUTXOsMsg::Handle(std::shared_ptr<Connection>& con)
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
