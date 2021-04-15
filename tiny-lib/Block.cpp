#include "pch.hpp"

#include <fmt/format.h>

#include "Block.hpp"
#include "SHA256.hpp"
#include "Utils.hpp"
#include "BinaryBuffer.hpp"

Block::Block(uint64_t version, const std::string& prevBlockHash, const std::string& markleHash, int64_t timestamp, uint8_t bits, uint64_t nonce,
	const std::vector<std::shared_ptr<Tx>>& txs)
	: Version(version), PrevBlockHash(prevBlockHash), MerkleHash(MerkleHash), Timestamp(timestamp), Bits(bits), Nonce(nonce), Txs(txs)
{
}

std::string Block::Header(uint64_t nonce /*= 0*/) const
{
	uint64_t used_nonce = nonce == 0 ? Nonce : nonce;

	return fmt::format("{}{}{}{}{}{}", Version, PrevBlockHash, MerkleHash, Timestamp, Bits, used_nonce);
}

std::string Block::Id() const
{
	std::string header = Header();
	std::vector<uint8_t> header_vec(header.begin(), header.end());

	return Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(header_vec));
}

std::vector<uint8_t> Block::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(Version);

	buffer.Write(PrevBlockHash);
	buffer.Write(MerkleHash);

	buffer.Write(Timestamp);

	buffer.Write(Bits);

	buffer.Write(Nonce);

	buffer.Write(Txs.size());
	for (const auto& tx : Txs)
		buffer.Write(tx->Serialize());

	buffer.Write(Id());

	return buffer.GetBuffer();
}
