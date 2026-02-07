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
	std::string new_hostname;
	if (!buffer.read(new_hostname))
		return false;

	uint16_t new_port = 0;
	if (!buffer.read(new_port))
		return false;

	hostname = std::move(new_hostname);
	port = new_port;

	return true;
}

Opcode PeerAddMsg::get_opcode() const
{
	return Opcode::PeerAddMsg;
}
