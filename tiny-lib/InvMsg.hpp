#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "IMsg.hpp"

class InvMsg : public IMsg
{
public:
	InvMsg() = default;
	InvMsg(const std::vector<std::string>& blocks);

	std::vector<std::string> Blocks;

	void Handle(const std::shared_ptr<NetClient::Connection>& con) const override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:

};
