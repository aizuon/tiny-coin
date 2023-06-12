#pragma once
#include "P2P/IMsg.hpp"
#include "Enums.hpp"

class PeerHelloMsg : public IMsg
{
public:
	NodeType NodeType;

	~PeerHelloMsg() override = default;

	void Handle(std::shared_ptr<Connection> con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;
};
