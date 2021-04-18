#pragma once
#include <cstdint>
#include <string>

#include "IHandleable.hpp"
#include "ISerializable.hpp"
#include "IDeserializable.hpp"

class AddPeerMsg : public IHandleable, public ISerializable, public IDeserializable
{
public:
	AddPeerMsg() = default;
	AddPeerMsg(const std::string& peerHostname);

	std::string PeerHostname;

	void Handle(NetClient::ConnectionHandle con_handle) const override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

private:
	static constexpr uint64_t ChunkSize = 50;
};