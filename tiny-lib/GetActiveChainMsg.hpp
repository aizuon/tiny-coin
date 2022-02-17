#pragma once
#include "IMsg.hpp"

class GetActiveChainMsg : public IMsg
{
public:
	~GetActiveChainMsg() = default;

	void Handle(std::shared_ptr<Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;
};
