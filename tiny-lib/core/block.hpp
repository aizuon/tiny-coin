#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "util/i_deserializable.hpp"
#include "util/i_serializable.hpp"
#include "core/tx.hpp"

class Block : public ISerializable, public IDeserializable
{
public:
	Block() = default;
	Block(uint64_t version, const std::string& prev_block_hash, const std::string& merkle_hash, int64_t timestamp,
		uint8_t bits, uint64_t nonce,
		const std::vector<std::shared_ptr<Tx>>& txs);

	uint64_t version = 0;

	std::string prev_block_hash;
	std::string merkle_hash;

	int64_t timestamp = -1;

	uint8_t bits = 0;

	uint64_t nonce = 0;

	std::vector<std::shared_ptr<Tx>> txs;

	BinaryBuffer header(std::optional<uint64_t> nonce = std::nullopt) const;

	std::string id() const;

	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	bool operator==(const Block& obj) const;

private:
	auto tied() const
	{
		return std::tie(version, prev_block_hash, merkle_hash, timestamp, bits, nonce);
	}
};
