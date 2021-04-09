#pragma once
#include <cstdint>
#include <string>
#include <memory>

#include "TxOutPoint.hpp"

class UnspentTxOut
{
public:
	UnspentTxOut(int64_t value, std::string toAddress, std::shared_ptr<TxOutPoint> txOut, bool isCoinbase, int64_t height)
		: Value(value), ToAddress(toAddress), TxOut(txOut), IsCoinbase(isCoinbase), Height(height)
	{

	}

	const int64_t Value;
	const std::string ToAddress;

	const std::shared_ptr<TxOutPoint> TxOut;

	const bool IsCoinbase;

	const int64_t Height;
};