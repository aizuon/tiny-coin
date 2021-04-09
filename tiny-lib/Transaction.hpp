#pragma once
#include <cstdint>
#include <vector>
#include <memory>

#include "ISerializable.hpp"
#include "TxIn.hpp"
#include "TxOut.hpp"

class Transaction : public ISerializable
{
public:
	Transaction(const std::vector<std::shared_ptr<TxIn>>& txIns, const std::vector<std::shared_ptr<TxOut>>& txOuts, int64_t lockTime)
		: TxIns(txIns), TxOuts(txOuts), LockTime(lockTime)
	{

	}

	std::vector<std::shared_ptr<TxIn>> TxIns;
	std::vector<std::shared_ptr<TxOut>> TxOuts;

	int64_t LockTime;

	bool IsCoinbase() const;

	std::vector<uint8_t> Serialize() const override;

	static std::shared_ptr<Transaction> CreateCoinbase(const std::string& PayToAddr, uint64_t value, int64_t height);
};