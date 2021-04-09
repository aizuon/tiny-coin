#pragma once
#include <cstdint>
#include <vector>
#include <memory>

#include "TxIn.h"
#include "TxOut.h"

class Transaction
{
public:
	Transaction(std::shared_ptr<std::vector<std::shared_ptr<TxIn>>> txIns, std::shared_ptr <std::vector<std::shared_ptr<TxOut>>> txOuts, int64_t lockTime)
		: TxIns(txIns), TxOuts(txOuts), LockTime(lockTime)
	{

	}

	std::shared_ptr<std::vector<std::shared_ptr<TxIn>>> TxIns;
	std::shared_ptr<std::vector<std::shared_ptr<TxOut>>> TxOuts;

	int64_t LockTime;

	bool IsCoinbase();

	static std::shared_ptr<Transaction> CreateCoinbase(const std::string& PayToAddr, uint64_t value, int64_t height);
};