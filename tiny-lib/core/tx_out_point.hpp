#pragma once
#include <cstdint>
#include <string>
#include <tuple>

#include "util/i_deserializable.hpp"
#include "util/i_serializable.hpp"

class TxOutPoint : public ISerializable, public IDeserializable
{
public:
	TxOutPoint() = default;
	TxOutPoint(const std::string& tx_id, int64_t tx_out_idx);

	std::string tx_id;
	int64_t tx_out_idx = -1;

	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	bool operator==(const TxOutPoint& obj) const;

private:
	auto tied() const
	{
		return std::tie(tx_id, tx_out_idx);
	}
};
