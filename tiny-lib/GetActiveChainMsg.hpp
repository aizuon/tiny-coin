#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "IMsg.hpp"
#include "Block.hpp"

class GetActiveChainMsg : public IMsg
{
public:
	GetActiveChainMsg() = default;
	GetActiveChainMsg(const std::vector<std::shared_ptr<Block>>& activeChain);

	std::vector<std::shared_ptr<Block>> ActiveChain;

	void Handle(NetClient::ConnectionHandle con_handle) const override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:

};
