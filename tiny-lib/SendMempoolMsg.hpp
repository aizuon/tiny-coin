#pragma once
#include <vector>
#include <string>

#include "IMsg.hpp"

class SendMempoolMsg : public IMsg
{
public:
	std::vector<std::string> Mempool;

	void Handle(std::shared_ptr<Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;
};
