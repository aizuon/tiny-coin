#include "net/peer_add_msg.hpp"

#include "net/net_client.hpp"

PeerAddMsg::PeerAddMsg(const std::string& hostname, uint16_t port)
	: hostname(hostname), port(port)
{}

void PeerAddMsg::handle([[maybe_unused]] const std::shared_ptr<Connection>& con)
{
	NetClient::connect(hostname, port);
}

BinaryBuffer PeerAddMsg::serialize() const
{
	BinaryBuffer buffer;

	buffer.write(hostname);
	buffer.write(port);

	return buffer;
}

bool PeerAddMsg::deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	if (!buffer.read(hostname))
	{
		*this = std::move(copy);

		return false;
	}
	if (!buffer.read(port))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

Opcode PeerAddMsg::get_opcode() const
{
	return Opcode::PeerAddMsg;
}
