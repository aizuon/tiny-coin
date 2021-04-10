#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#include "Tx.hpp"

class Block
{
public:
	Block(uint64_t version, const std::string& prevBlockHash, const std::string& markleHash, int64_t timestamp, uint8_t bits, uint64_t nonce,
		const std::vector<std::shared_ptr<Tx>>& txs);

	uint64_t Version;

	std::string PrevBlockHash;
	std::string MerkleHash;

	int64_t Timestamp;

	uint8_t Bits;

	uint64_t Nonce;

	std::vector<std::shared_ptr<Tx>> Txs;

	std::string Header(uint64_t nonce = 0);

	std::string Id();
};
