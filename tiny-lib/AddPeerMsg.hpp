#pragma once
#include <cstdint>
#include <string>

#include "IMsg.hpp"

class AddPeerMsg : public IMsg
{
public:
	AddPeerMsg() = default;
	AddPeerMsg(const std::string& gostname, uint16_t port);

	std::string Hostname;
	uint16_t Port;

	void Handle(const std::shared_ptr<NetClient::Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:
	static constexpr uint64_t ChunkSize = 50;
};