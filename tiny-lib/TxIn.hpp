#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <tuple>

#include "ISerializable.hpp"
#include "IDeserializable.hpp"
#include "TxOutPoint.hpp"

class TxIn : public ISerializable, public IDeserializable
{
public:
	TxIn() = default;
	TxIn(const std::shared_ptr<TxOutPoint>& toSpend, const std::vector<uint8_t>& unlockSig, const std::vector<uint8_t>& unlockPk, int32_t sequence);

	std::shared_ptr<TxOutPoint> ToSpend;

	std::vector<uint8_t> UnlockSig;
	std::vector<uint8_t> UnlockPubKey;

	int32_t Sequence = -1;

	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	bool operator==(const TxIn& obj) const;

private:
	auto tied() const
	{
		return std::tie(UnlockSig, UnlockPubKey, Sequence);
	}
};