#pragma once
#include "P2P/IMsg.hpp"

class GetUTXOsMsg : public IMsg
{
public:
	~GetUTXOsMsg() override = default;

	void Handle(std::shared_ptr<Connection> con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;
};
