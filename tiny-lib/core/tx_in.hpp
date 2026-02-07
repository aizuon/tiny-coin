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
	static constexpr int32_t SEQUENCE_FINAL = -1;
	static constexpr int32_t SEQUENCE_RBF = -3;

	static constexpr uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = 1u << 31;
	static constexpr uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = 1u << 22;
	static constexpr uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000FFFFu;
	static constexpr uint32_t SEQUENCE_LOCKTIME_GRANULARITY = 9;

	TxIn() = default;
	TxIn(std::shared_ptr<TxOutPoint> to_spend, std::vector<uint8_t> unlock_sig,
		std::vector<uint8_t> unlock_pub_key, int32_t sequence);

	std::shared_ptr<TxOutPoint> to_spend;

	std::vector<uint8_t> unlock_sig;
	std::vector<uint8_t> unlock_pub_key;

	int32_t sequence = SEQUENCE_FINAL;

	bool has_relative_locktime() const;

	bool is_time_based_locktime() const;

	uint32_t relative_locktime_value() const;

	static constexpr int32_t encode_relative_blocks(uint16_t blocks)
	{
		return static_cast<int32_t>(static_cast<uint32_t>(blocks) & SEQUENCE_LOCKTIME_MASK);
	}

	static constexpr int32_t encode_relative_time(uint16_t units_512s)
	{
		return static_cast<int32_t>(
			(static_cast<uint32_t>(units_512s) & SEQUENCE_LOCKTIME_MASK)
			| SEQUENCE_LOCKTIME_TYPE_FLAG);
	}

	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	bool operator==(const TxIn& obj) const;

private:
	auto tied() const
	{
		return std::tie(unlock_sig, unlock_pub_key, sequence);
	}
};
