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

	void Handle(NetClient::ConnectionHandle con_handle) const override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:
	static constexpr uint64_t ChunkSize = 50;
};