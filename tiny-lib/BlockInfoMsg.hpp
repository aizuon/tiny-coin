#pragma once
#include <cstdint>
#include <memory>

#include "Block.hpp"
#include "IMsg.hpp"

class BlockInfoMsg : public IMsg
{
public:
	BlockInfoMsg() = default;
	BlockInfoMsg(const std::shared_ptr<Block>& block);

	~BlockInfoMsg() override = default;

	std::shared_ptr<Block> Block;

	void Handle(std::shared_ptr<Connection> con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;
};
