#include "pch.hpp"

#include <fmt/format.h>

#include "Block.hpp"
#include "SHA256d.hpp"

Block::Block(uint64_t version, const std::string& prevBlockHash, const std::string& markleHash, int64_t timestamp, uint8_t bits, int64_t nonce,
	const std::vector<std::shared_ptr<Tx>>& txs)
	: Version(version), PrevBlockHash(prevBlockHash), MerkleHash(MerkleHash), Timestamp(timestamp), Bits(bits), Nonce(nonce), Txs(txs)
{
}

std::string Block::Header(int64_t nonce /*= -1*/)
{
	int64_t used_nonce = nonce == -1 ? Nonce : nonce;

	return fmt::format("{}{}{}{}{}{}", Version, PrevBlockHash, MerkleHash, Timestamp, Bits, used_nonce);
}

std::string Block::Id()
{
	std::string header = Header();
	std::vector<uint8_t> header_vec(header.begin(), header.end());

	return SHA256d::BinaryHashToString(SHA256d::HashBinary(header_vec));
}
