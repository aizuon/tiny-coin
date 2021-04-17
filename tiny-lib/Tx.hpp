#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#include "ISerializable.hpp"

class TxIn;
class TxOut;
class UnspentTxOut;

class Tx : public ISerializable
{
public:
	Tx(const std::vector<std::shared_ptr<TxIn>>& txIns, const std::vector<std::shared_ptr<TxOut>>& txOuts, int64_t lockTime);

	std::vector<std::shared_ptr<TxIn>> TxIns;
	std::vector<std::shared_ptr<TxOut>> TxOuts;

	int64_t LockTime;

	bool IsCoinbase() const;

	std::string Id() const;

	void ValidateBasics(bool coinbase = false) const;

	std::vector<uint8_t> Serialize() const override;

	static std::shared_ptr<Tx> CreateCoinbase(const std::string& PayToAddr, uint64_t value, int64_t height);

	struct ValidateRequest
	{
		bool AsCoinbase = false;
		bool Allow_UTXO_FromMempool = true;
		std::vector<std::shared_ptr<Tx>> SiblingsInBlock;
	};

	static void Validate(const std::shared_ptr<Tx>& tx, const ValidateRequest& req);

	static void ValidateSignatureForSpend(const std::shared_ptr<TxIn>& txIn, const std::shared_ptr<UnspentTxOut>& utxo, const std::shared_ptr<Tx>& tx);
};