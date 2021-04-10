#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#include "Tx.hpp"

class Block
{
public:
	Block(uint64_t version, const std::string& prevBlockHash, const std::string& markleHash, int64_t timestamp, uint8_t bits, int64_t nonce,
		const std::vector<std::shared_ptr<Tx>>& txs);

	uint64_t Version;

	std::string PrevBlockHash;
	std::string MerkleHash;

	int64_t Timestamp;

	uint8_t Bits;

	int64_t Nonce;

	std::vector<std::shared_ptr<Tx>> Txs;

	std::string Header(int64_t nonce = -1);

	std::string Id();
};
