#include "pch.hpp"
#include "P2P/GetMempoolMsg.hpp"

#include "P2P/NetClient.hpp"
#include "P2P/SendMempoolMsg.hpp"

void GetMempoolMsg::Handle(std::shared_ptr<Connection> con)
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
