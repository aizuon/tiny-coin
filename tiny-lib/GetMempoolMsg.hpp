#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "IMsg.hpp"

class GetMempoolMsg : public IMsg
{
public:
	GetMempoolMsg() = default;
	GetMempoolMsg(const std::vector<std::string>& mempool);

	std::vector<std::string> Mempool;

	void Handle(const std::shared_ptr<NetClient::Connection>& con) const override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:

};
