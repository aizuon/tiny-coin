#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "IMsg.hpp"
#include "Block.hpp"

class InvMsg : public IMsg
{
public:
	InvMsg() = default;
	InvMsg(const std::vector<std::shared_ptr<Block>>& blocks);

	std::vector<std::shared_ptr<Block>> Blocks;

	void Handle(const std::shared_ptr<NetClient::Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:

};
