#pragma once
#include <cstdint>
#include <string>

class TxOut
{
public:
	TxOut(uint64_t value, const std::string& toAddress)
		: Value(value), ToAddress(toAddress)
	{

	}

	uint64_t Value;

	std::string ToAddress;
};