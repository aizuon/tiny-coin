#include "pch.hpp"

#include "GetMempoolMsg.hpp"
#include "SendMempoolMsg.hpp"

void GetMempoolMsg::Handle(const std::shared_ptr<NetClient::Connection>& con)
{
	NetClient::SendMsgAsync(con, SendMempoolMsg());
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
