#pragma once
#include <cstdint>
#include <string>

#include "IMsg.hpp"

class PeerAddMsg : public IMsg
{
public:
	PeerAddMsg() = default;
	PeerAddMsg(const std::string& hostname, uint16_t port);

	std::string Hostname;
	uint16_t Port = 0;

	void Handle(std::shared_ptr<Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;
};
