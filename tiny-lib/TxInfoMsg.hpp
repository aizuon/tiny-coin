#pragma once
#include <cstdint>
#include <memory>

#include "IMsg.hpp"
#include "Tx.hpp"

class TxInfoMsg : public IMsg
{
public:
	TxInfoMsg() = default;
	TxInfoMsg(const std::shared_ptr<Tx>& tx);

	std::shared_ptr<Tx> Tx;

	void Handle(const std::shared_ptr<NetClient::Connection>& con) const override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:

};