#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "net/i_msg.hpp"
#include "core/tx_out_point.hpp"
#include "core/unspent_tx_out.hpp"

class SendUTXOsMsg : public IMsg
{
public:
	std::unordered_map<std::shared_ptr<TxOutPoint>, std::shared_ptr<UTXO>> utxo_map;

	~SendUTXOsMsg() override = default;

	void handle(const std::shared_ptr<Connection>& con) override;
	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	Opcode get_opcode() const override;
};
