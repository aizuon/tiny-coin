#include "net/get_active_chain_msg.hpp"

#include "net/net_client.hpp"
#include "net/send_active_chain_msg.hpp"

void GetActiveChainMsg::handle(const std::shared_ptr<Connection>& con)
{
	NetClient::send_msg(con, SendActiveChainMsg());
}

BinaryBuffer GetActiveChainMsg::serialize() const
{
	BinaryBuffer buffer;

	return buffer;
}

bool GetActiveChainMsg::deserialize([[maybe_unused]] BinaryBuffer& buffer)
{
	return true;
}

Opcode GetActiveChainMsg::get_opcode() const
{
	return Opcode::GetActiveChainMsg;
}
