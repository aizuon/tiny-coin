#pragma once
#include <cstdint>
#include <memory>
#include <tuple>
#include <vector>

#include "util/i_deserializable.hpp"
#include "util/i_serializable.hpp"
#include "core/tx_out_point.hpp"

class TxIn : public ISerializable, public IDeserializable
{
public:
	TxIn() = default;
	TxIn(std::shared_ptr<TxOutPoint> to_spend, std::vector<uint8_t> unlock_sig,
		std::vector<uint8_t> unlock_pub_key, int32_t sequence);

	std::shared_ptr<TxOutPoint> to_spend;

	std::vector<uint8_t> unlock_sig;
	std::vector<uint8_t> unlock_pub_key;

	int32_t sequence = -1;

	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	bool operator==(const TxIn& obj) const;

private:
	auto tied() const
	{
		return std::tie(unlock_sig, unlock_pub_key, sequence);
	}
};
