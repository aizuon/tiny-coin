#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <tuple>

#include "util/i_deserializable.hpp"
#include "util/i_serializable.hpp"

class TxOutPoint : public ISerializable, public IDeserializable
{
public:
	TxOutPoint() = default;
	TxOutPoint(std::string tx_id, int64_t tx_out_idx);

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

struct TxOutPointHash
{
	size_t operator()(const std::shared_ptr<TxOutPoint>& p) const
	{
		if (!p) return 0;
		size_t h1 = std::hash<std::string>{}(p->tx_id);
		size_t h2 = std::hash<int64_t>{}(p->tx_out_idx);
		h1 ^= h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
		return h1;
	}
};

struct TxOutPointEqual
{
	bool operator()(const std::shared_ptr<TxOutPoint>& a, const std::shared_ptr<TxOutPoint>& b) const
	{
		if (a == b) return true;
		if (!a || !b) return false;
		return *a == *b;
	}
};
