#include "pch.hpp"

#include "AddPeerMsg.hpp"

AddPeerMsg::AddPeerMsg(const std::string& peerHostname)
	: PeerHostname(peerHostname)
{

}

void AddPeerMsg::Handle(const std::shared_ptr<NetClient::Connection>& con)
{
	//TODO
}

BinaryBuffer AddPeerMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(PeerHostname);

	return buffer;
}

bool AddPeerMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	if (!buffer.Read(PeerHostname))
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
