#pragma once
#include <cstdint>
#include <string>

class TxOut
{
public:
	TxOut(uint64_t value, std::string toAddress)
		: Value(value), ToAddress(toAddress)
	{

	}

	const uint64_t Value;

	const std::string ToAddress;
};