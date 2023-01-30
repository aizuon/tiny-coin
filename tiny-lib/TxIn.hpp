#pragma once
#include <cstdint>
#include <memory>
#include <tuple>
#include <vector>

#include "IDeserializable.hpp"
#include "ISerializable.hpp"
#include "TxOutPoint.hpp"

class TxIn : public ISerializable, public IDeserializable
{
public:
	TxIn() = default;
	TxIn(std::shared_ptr<TxOutPoint> to_spend, const std::vector<uint8_t>& unlock_sig,
	     const std::vector<uint8_t>& unlock_pub_key, int32_t sequence);

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
