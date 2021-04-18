#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "IMsg.hpp"
#include "Block.hpp"

class SendActiveChainMsg : public IMsg
{
public:
	std::vector<std::shared_ptr<Block>> ActiveChain;

	void Handle(const std::shared_ptr<NetClient::Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:

};
