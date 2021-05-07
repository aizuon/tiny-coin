#include "pch.hpp"
#include "PeerAddMsg.hpp"

#include "NetClient.hpp"

PeerAddMsg::PeerAddMsg(const std::string& hostname, uint16_t port)
	: Hostname(hostname), Port(port)
{
}

void PeerAddMsg::Handle(std::shared_ptr<Connection>& con)
{
	NetClient::Connect(Hostname, Port);
}

BinaryBuffer PeerAddMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(Hostname);
	buffer.Write(Port);

	return buffer;
}

bool PeerAddMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	if (!buffer.Read(Hostname))
	{
		*this = std::move(copy);

		return false;
	}
	if (!buffer.Read(Port))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

Opcode PeerAddMsg::GetOpcode() const
{
	return Opcode::PeerAddMsg;
}
