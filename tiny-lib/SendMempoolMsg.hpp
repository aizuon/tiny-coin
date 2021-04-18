#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "IMsg.hpp"

class SendMempoolMsg : public IMsg
{
public:
	std::vector<std::string> Mempool;

	void Handle(const std::shared_ptr<NetClient::Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:

};
