#include "net/get_mempool_msg.hpp"

#include "net/net_client.hpp"
#include "net/send_mempool_msg.hpp"

void GetMempoolMsg::handle(const std::shared_ptr<Connection>& con)
{
	NetClient::send_msg(con, SendMempoolMsg());
}

BinaryBuffer GetMempoolMsg::serialize() const
{
	BinaryBuffer buffer;

	return buffer;
}

bool GetMempoolMsg::deserialize(BinaryBuffer& buffer)
{
	return true;
}

Opcode GetMempoolMsg::get_opcode() const
{
	return Opcode::GetMempoolMsg;
}
