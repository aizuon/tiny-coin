#include "pch.hpp"

#include <algorithm>

#include "GetUTXOsMsg.hpp"
#include "NetClient.hpp"
#include "SendUTXOsMsg.hpp"

void GetUTXOsMsg::Handle(std::shared_ptr<NetClient::Connection>& con)
{
	NetClient::SendMsgAsync(con, SendUTXOsMsg());
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
