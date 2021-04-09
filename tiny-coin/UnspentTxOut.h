#pragma once
#include <cstdint>
#include <string>
#include <memory>

#include "TxOutPoint.h"

class UnspentTxOut
{
public:
	UnspentTxOut(int64_t value, const std::string& toAddress, std::shared_ptr<TxOutPoint> txOut, bool isCoinbase, int64_t height)
		: Value(value), ToAddress(toAddress), TxOut(txOut), IsCoinbase(isCoinbase), Height(height)
	{

	}

	int64_t Value;
	std::string ToAddress;

	std::shared_ptr<TxOutPoint> TxOut;

	bool IsCoinbase;

	int64_t Height;
};