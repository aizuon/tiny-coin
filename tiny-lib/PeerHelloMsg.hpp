#pragma once
#include "IMsg.hpp"
#include "Enums.hpp"

class PeerHelloMsg : public IMsg
{
public:
	NodeType NodeType;

	void Handle(std::shared_ptr<Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;
};
