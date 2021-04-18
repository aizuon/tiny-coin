#include "pch.hpp"

#include <algorithm>

#include "GetUTXOsMsg.hpp"
#include "SendUTXOsMsg.hpp"
#include "NetClient.hpp"

void GetUTXOsMsg::Handle(const std::shared_ptr<NetClient::Connection>& con)
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
