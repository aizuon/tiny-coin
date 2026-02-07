#pragma once
#include <cstdint>
#include <string>

#include "net/i_msg.hpp"

class GetBlockMsg : public IMsg
{
public:
	GetBlockMsg() = default;
	GetBlockMsg(const std::string& from_block_id);

	~GetBlockMsg() override = default;

	std::string from_block_id;

	void handle(const std::shared_ptr<Connection>& con) override;
	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	Opcode get_opcode() const override;

private:
	static constexpr uint32_t CHUNK_SIZE = 50;
};
