#pragma once
#include <cstdint>
#include <memory>

#include "TxOutPoint.h"

class TxIn
{
public:
	TxIn(std::shared_ptr<TxOutPoint> toSpend, std::shared_ptr<std::vector<uint8_t>> unlockSig, std::shared_ptr<std::vector<uint8_t>> unlockPk, int64_t sequence)
		: ToSpend(toSpend), UnlockSig(unlockSig), UnlockPk(unlockPk), Sequence(sequence)
	{

	}

	std::shared_ptr<TxOutPoint> ToSpend;

	std::shared_ptr<std::vector<uint8_t>> UnlockSig;
	std::shared_ptr<std::vector<uint8_t>> UnlockPk;

	int64_t Sequence;
};