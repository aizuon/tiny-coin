#pragma once
#include <cstdint>
#include <string>

#include "net/i_msg.hpp"

class PeerAddMsg : public IMsg
{
public:
	PeerAddMsg() = default;
	PeerAddMsg(const std::string& hostname, uint16_t port);

	~PeerAddMsg() override = default;

	std::string hostname;
	uint16_t port = 0;

	void handle(const std::shared_ptr<Connection>& con) override;
	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	Opcode get_opcode() const override;
};
