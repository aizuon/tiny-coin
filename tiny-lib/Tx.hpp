#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "IDeserializable.hpp"
#include "ISerializable.hpp"
#include "TxIn.hpp"
#include "TxOut.hpp"

class UnspentTxOut;

class Tx : public ISerializable, public IDeserializable
{
public:
	Tx() = default;
	Tx(const std::vector<std::shared_ptr<TxIn>>& tx_ins, const std::vector<std::shared_ptr<TxOut>>& tx_outs,
	   int64_t lock_time);

	std::vector<std::shared_ptr<TxIn>> TxIns;
	std::vector<std::shared_ptr<TxOut>> TxOuts;

	int64_t LockTime = 0;

	bool IsCoinbase() const;

	std::string Id() const;

	void ValidateBasics(bool coinbase = false) const;

	struct ValidateRequest
	{
		bool AsCoinbase = false;
		bool Allow_UTXO_FromMempool = true;
		std::vector<std::shared_ptr<Tx>> SiblingsInBlock;
	};

	void Validate(const ValidateRequest& req) const;

	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	static std::shared_ptr<Tx> CreateCoinbase(const std::string& pay_to_addr, uint64_t value, int64_t height);

	bool operator==(const Tx& obj) const;

private:
	void ValidateSignatureForSpend(std::shared_ptr<TxIn> tx_in, std::shared_ptr<UnspentTxOut> utxo) const;

	auto tied() const
	{
		return std::tie(LockTime);
	}
};
