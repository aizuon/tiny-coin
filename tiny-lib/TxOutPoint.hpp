#pragma once
#include <string>

class TxOutPoint
{
public:
	TxOutPoint(std::string txId, int64_t txOutId)
		: TxId(txId), TxOutId(txOutId)
	{

	}

	const std::string TxId;
	const int64_t TxOutId;
};