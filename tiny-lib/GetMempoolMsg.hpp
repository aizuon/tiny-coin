#pragma once
#include "IMsg.hpp"

class GetMempoolMsg : public IMsg
{
public:
	~GetMempoolMsg() override = default;

	void Handle(std::shared_ptr<Connection> con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;
};
