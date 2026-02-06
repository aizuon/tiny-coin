#include "net/get_utxos_msg.hpp"

#include "net/net_client.hpp"
#include "net/send_utxos_msg.hpp"

void GetUTXOsMsg::handle(const std::shared_ptr<Connection>& con)
{
	NetClient::send_msg(con, SendUTXOsMsg());
}

BinaryBuffer GetUTXOsMsg::serialize() const
{
	BinaryBuffer buffer;

	return buffer;
}

bool GetUTXOsMsg::deserialize([[maybe_unused]] BinaryBuffer& buffer)
{
	return true;
}

Opcode GetUTXOsMsg::get_opcode() const
{
	return Opcode::GetUTXOsMsg;
}
