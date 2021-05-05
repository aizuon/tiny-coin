#pragma once
#include <cstdint>
#include <string>

#include "IMsg.hpp"

class GetBlockMsg : public IMsg
{
public:
	GetBlockMsg() = default;
	GetBlockMsg(const std::string& fromBlockId);

	std::string FromBlockId;

	void Handle(std::shared_ptr<Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const override;

private:
	static constexpr uint64_t ChunkSize = 50;
};
