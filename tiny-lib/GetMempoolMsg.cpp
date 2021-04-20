#include "pch.hpp"

#include "GetMempoolMsg.hpp"
#include "NetClient.hpp"
#include "SendMempoolMsg.hpp"

void GetMempoolMsg::Handle(std::shared_ptr<Connection>& con)
{
	NetClient::SendMsg(con, SendMempoolMsg());
}

BinaryBuffer GetMempoolMsg::Serialize() const
{
	BinaryBuffer buffer;

	return buffer;
}

bool GetMempoolMsg::Deserialize(BinaryBuffer& buffer)
{
	return true;
}

Opcode GetMempoolMsg::GetOpcode() const
{
	return Opcode::GetMempoolMsg;
}
