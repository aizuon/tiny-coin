#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include "IMsg.hpp"
#include "TxOutPoint.hpp"
#include "UnspentTxOut.hpp"

class GetUTXOsMsg : public IMsg
{
public:
	GetUTXOsMsg() = default;
	GetUTXOsMsg(const std::unordered_map<std::shared_ptr<TxOutPoint>, std::shared_ptr<UnspentTxOut>>& utxoMap);

	std::unordered_map<std::shared_ptr<TxOutPoint>, std::shared_ptr<UnspentTxOut>> UTXO_Map;

	void Handle(NetClient::ConnectionHandle con_handle) const override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	Opcode GetOpcode() const;

private:

};
