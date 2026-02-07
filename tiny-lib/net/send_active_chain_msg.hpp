#pragma once
#include <cstdint>
#include <vector>

#include "core/block.hpp"
#include "net/i_msg.hpp"

class SendActiveChainMsg : public IMsg
{
public:
	std::vector<std::shared_ptr<Block>> active_chain;

	~SendActiveChainMsg() override = default;

	void handle(const std::shared_ptr<Connection>& con) override;
	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	Opcode get_opcode() const override;
};
