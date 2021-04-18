#pragma once
#include <cstdint>
#include <string>

#include "IMsg.hpp"

class AddPeerMsg : public IMsg
{
public:
	AddPeerMsg() = default;
	AddPeerMsg(const std::string& peerHostname);

	std::string PeerHostname;

	void Handle(NetClient::ConnectionHandle con_handle) const override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:
	static constexpr uint64_t ChunkSize = 50;
};