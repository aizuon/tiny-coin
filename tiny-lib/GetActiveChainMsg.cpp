#include "pch.hpp"

#include "GetActiveChainMsg.hpp"
#include "NetClient.hpp"
#include "SendActiveChainMsg.hpp"

void GetActiveChainMsg::Handle(std::shared_ptr<NetClient::Connection>& con)
{
	NetClient::SendMsgAsync(con, SendActiveChainMsg());
}

BinaryBuffer GetActiveChainMsg::Serialize() const
{
	BinaryBuffer buffer;

	return buffer;
}

bool GetActiveChainMsg::Deserialize(BinaryBuffer& buffer)
{
	return true;
}

Opcode GetActiveChainMsg::GetOpcode() const
{
	return Opcode::GetActiveChainMsg;
}
