#pragma once
#include "IMsg.hpp"

class GetMempoolMsg : public IMsg
{
public:
	void Handle(std::shared_ptr<NetClient::Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:

};
