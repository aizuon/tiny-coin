#pragma once
#include "net/i_msg.hpp"

class GetActiveChainMsg : public IMsg
{
public:
	~GetActiveChainMsg() override = default;

	void handle(const std::shared_ptr<Connection>& con) override;
	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	Opcode get_opcode() const override;
};
