#pragma once
#include <cstdint>
#include <memory>

#include "net/i_msg.hpp"
#include "core/tx.hpp"

class TxInfoMsg : public IMsg
{
public:
	TxInfoMsg() = default;
	TxInfoMsg(const std::shared_ptr<Tx>& tx);

	~TxInfoMsg() override = default;

	std::shared_ptr<Tx> tx;

	void handle(const std::shared_ptr<Connection>& con) override;
	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	Opcode get_opcode() const override;
};
