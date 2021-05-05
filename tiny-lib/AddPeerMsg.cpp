#include "pch.hpp"

#include "AddPeerMsg.hpp"
#include "NetClient.hpp"

AddPeerMsg::AddPeerMsg(const std::string& hostname, uint16_t port)
	: Hostname(hostname), Port(port)
{
}

void AddPeerMsg::Handle(std::shared_ptr<Connection>& con)
{
	NetClient::Connect(Hostname, Port);
}

BinaryBuffer AddPeerMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(Hostname);
	buffer.Write(Port);

	return buffer;
}

bool AddPeerMsg::Deserialize(BinaryBuffer& buffer)
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

Opcode AddPeerMsg::GetOpcode() const
{
	return Opcode::AddPeerMsg;
}
