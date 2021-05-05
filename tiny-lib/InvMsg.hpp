#pragma once
#include <vector>

#include "IMsg.hpp"
#include "Block.hpp"

class InvMsg : public IMsg
{
public:
	InvMsg() = default;
	InvMsg(const std::vector<std::shared_ptr<Block>>& blocks);

	std::vector<std::shared_ptr<Block>> Blocks;

	void Handle(std::shared_ptr<Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;

private:
};
