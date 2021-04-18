#include "pch.hpp"

#include "AddPeerMsg.hpp"
#include "BinaryBuffer.hpp"

AddPeerMsg::AddPeerMsg(const std::string& peerHostname)
	: PeerHostname(peerHostname)
{

}

void AddPeerMsg::Handle(NetClient::ConnectionHandle con_handle) const
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
