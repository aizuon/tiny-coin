#pragma once
#include <string>
#include <vector>

#include "net/i_msg.hpp"

class SendMempoolMsg : public IMsg
{
public:
	std::vector<std::string> mempool;

	~SendMempoolMsg() override = default;

	void handle(const std::shared_ptr<Connection>& con) override;
	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	Opcode get_opcode() const override;
};
