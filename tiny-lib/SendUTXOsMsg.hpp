#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include "IMsg.hpp"
#include "TxOutPoint.hpp"
#include "UnspentTxOut.hpp"

class SendUTXOsMsg : public IMsg
{
public:
	std::unordered_map<std::shared_ptr<TxOutPoint>, std::shared_ptr<UnspentTxOut>> UTXO_Map;

	void Handle(std::shared_ptr<NetClient::Connection>& con) override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:

};
