#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <tuple>

#include "ISerializable.hpp"
#include "IDeserializable.hpp"

class TxOutPoint : public ISerializable, public IDeserializable
{
public:
	TxOutPoint() = default;
	TxOutPoint(const std::string& txId, int64_t txOutIdx);

	std::string TxId;
	int64_t TxOutIdx = -1;

	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	bool operator==(const TxOutPoint& obj) const;

private:
	auto tied() const
	{
		return std::tie(TxId, TxOutIdx);
	}
};