#pragma once
#include <cstdint>
#include <vector>

#include "Block.hpp"
#include "IMsg.hpp"

class SendActiveChainMsg : public IMsg
{
public:
	std::vector<std::shared_ptr<Block>> ActiveChain;

	void Handle(std::shared_ptr<Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;

private:
};
