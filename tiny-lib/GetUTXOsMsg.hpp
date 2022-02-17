#pragma once
#include "IMsg.hpp"

class GetUTXOsMsg : public IMsg
{
public:
	~GetUTXOsMsg() = default;

	void Handle(std::shared_ptr<Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;
};
