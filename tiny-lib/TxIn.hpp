#pragma once
#include <cstdint>
#include <memory>

#include "TxOutPoint.hpp"

class TxIn
{
public:
	TxIn(std::shared_ptr<TxOutPoint> toSpend, const std::vector<uint8_t>& unlockSig, const std::vector<uint8_t>& unlockPk, int64_t sequence)
		: ToSpend(toSpend), UnlockSig(unlockSig), UnlockPk(unlockPk), Sequence(sequence)
	{

	}

	const std::shared_ptr<TxOutPoint> ToSpend;

	const std::vector<uint8_t> UnlockSig;
	const std::vector<uint8_t> UnlockPk;

	const int64_t Sequence;
};