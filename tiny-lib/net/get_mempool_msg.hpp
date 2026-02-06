#pragma once
#include "net/i_msg.hpp"

class GetMempoolMsg : public IMsg
{
public:
	~GetMempoolMsg() override = default;

	void handle(const std::shared_ptr<Connection>& con) override;
	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	Opcode get_opcode() const override;
};
