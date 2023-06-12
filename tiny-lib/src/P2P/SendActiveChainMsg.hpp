#pragma once
#include <cstdint>
#include <vector>

#include "BlockChain/Block.hpp"
#include "P2P/IMsg.hpp"

class SendActiveChainMsg : public IMsg
{
public:
	std::vector<std::shared_ptr<Block>> ActiveChain;

	~SendActiveChainMsg() override = default;

	void Handle(std::shared_ptr<Connection> con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;
};
