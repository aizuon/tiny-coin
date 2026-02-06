#pragma once
#include <cstdint>
#include <memory>

#include "core/block.hpp"
#include "net/i_msg.hpp"

class BlockInfoMsg : public IMsg
{
public:
	BlockInfoMsg() = default;
	BlockInfoMsg(const std::shared_ptr<Block>& block);

	~BlockInfoMsg() override = default;

	std::shared_ptr<Block> block;

	void handle(const std::shared_ptr<Connection>& con) override;
	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	Opcode get_opcode() const override;
};
