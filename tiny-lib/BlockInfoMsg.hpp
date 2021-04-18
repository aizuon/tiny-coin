#pragma once
#include <cstdint>
#include <memory>

#include "IMsg.hpp"
#include "Block.hpp"

class BlockInfoMsg : public IMsg
{
public:
	BlockInfoMsg() = default;
	BlockInfoMsg(const std::shared_ptr<Block>& block);

	std::shared_ptr<Block> Block;

	void Handle(const std::shared_ptr<NetClient::Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:

};