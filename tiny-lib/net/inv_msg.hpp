#pragma once
#include <vector>

#include "core/block.hpp"
#include "net/i_msg.hpp"

class InvMsg : public IMsg
{
public:
	InvMsg() = default;
	InvMsg(const std::vector<std::shared_ptr<Block>>& blocks);

	~InvMsg() override = default;

	std::vector<std::shared_ptr<Block>> blocks;

	void handle(const std::shared_ptr<Connection>& con) override;
	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	Opcode get_opcode() const override;
};
