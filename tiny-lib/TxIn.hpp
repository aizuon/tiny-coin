#pragma once
#include <cstdint>
#include <vector>
#include <memory>

#include "ISerializable.hpp"
#include "TxOutPoint.hpp"

class TxIn : public ISerializable
{
public:
	TxIn(std::shared_ptr<TxOutPoint> toSpend, const std::vector<uint8_t>& unlockSig, const std::vector<uint8_t>& unlockPk, int32_t sequence);

	std::shared_ptr<TxOutPoint> ToSpend;

	std::vector<uint8_t> UnlockSig;
	std::vector<uint8_t> UnlockPk;

	int32_t Sequence;

	std::vector<uint8_t> Serialize() const override;
};