#include "pch.hpp"
#include "P2P/GetActiveChainMsg.hpp"

#include "P2P/NetClient.hpp"
#include "P2P/SendActiveChainMsg.hpp"

void GetActiveChainMsg::Handle(std::shared_ptr<Connection> con)
{
	NetClient::SendMsg(con, SendActiveChainMsg());
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
