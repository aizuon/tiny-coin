#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "IDeserializable.hpp"
#include "ISerializable.hpp"
#include "Tx.hpp"

class Block : public ISerializable, public IDeserializable
{
public:
	Block() = default;
	Block(uint64_t version, const std::string& prevBlockHash, const std::string& markleHash, int64_t timestamp,
	      uint8_t bits, uint64_t nonce,
	      const std::vector<std::shared_ptr<Tx>>& txs);

	uint64_t Version = 0;

	std::string PrevBlockHash;
	std::string MerkleHash;

	int64_t Timestamp = -1;

	uint8_t Bits = 0;

	uint64_t Nonce = 0;

	std::vector<std::shared_ptr<Tx>> Txs;

	std::string Header(uint64_t nonce = 0) const;

	std::string Id() const;

	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	bool operator==(const Block& obj) const;

private:
	auto tied() const
	{
		return std::tie(Version, PrevBlockHash, MerkleHash, Timestamp, Bits, Nonce);
	}
};
