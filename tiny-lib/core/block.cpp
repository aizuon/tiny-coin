#include "core/block.hpp"

#include "crypto/sha256.hpp"
#include "util/utils.hpp"

Block::Block(uint64_t version, const std::string& prev_block_hash, const std::string& merkle_hash, int64_t timestamp,
	uint8_t bits, uint64_t nonce,
	const std::vector<std::shared_ptr<Tx>>& txs)
	: version(version), prev_block_hash(prev_block_hash), merkle_hash(merkle_hash), timestamp(timestamp), bits(bits),
	nonce(nonce), txs(txs)
{}

Block::Block(const Block& other)
	: version(other.version), prev_block_hash(other.prev_block_hash), merkle_hash(other.merkle_hash),
	timestamp(other.timestamp), bits(other.bits), nonce(other.nonce), txs(other.txs)
{
}

Block& Block::operator=(const Block& other)
{
	if (this != &other)
	{
		version = other.version;
		prev_block_hash = other.prev_block_hash;
		merkle_hash = other.merkle_hash;
		timestamp = other.timestamp;
		bits = other.bits;
		nonce = other.nonce;
		txs = other.txs;
		cached_id_.clear();
	}
	return *this;
}

Block::Block(Block&& other) noexcept
	: version(other.version), prev_block_hash(std::move(other.prev_block_hash)),
	merkle_hash(std::move(other.merkle_hash)), timestamp(other.timestamp),
	bits(other.bits), nonce(other.nonce), txs(std::move(other.txs))
{
}

Block& Block::operator=(Block&& other) noexcept
{
	if (this != &other)
	{
		version = other.version;
		prev_block_hash = std::move(other.prev_block_hash);
		merkle_hash = std::move(other.merkle_hash);
		timestamp = other.timestamp;
		bits = other.bits;
		nonce = other.nonce;
		txs = std::move(other.txs);
		cached_id_.clear();
	}
	return *this;
}

BinaryBuffer Block::header(std::optional<uint64_t> nonce /*= std::nullopt*/) const
{
	BinaryBuffer buffer = header_prefix();

	buffer.write(nonce.value_or(this->nonce));

	return buffer;
}

BinaryBuffer Block::header_prefix() const
{
	BinaryBuffer buffer;

	buffer.write(version);

	buffer.write(prev_block_hash);
	buffer.write(merkle_hash);

	buffer.write(timestamp);

	buffer.write(bits);

	return buffer;
}

std::string Block::id() const
{
	std::scoped_lock lock(cached_id_mutex_);

	if (cached_id_.empty())
		cached_id_ = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(header().get_buffer()));

	return cached_id_;
}

BinaryBuffer Block::serialize() const
{
	BinaryBuffer buffer = header();

	buffer.write_size(static_cast<uint32_t>(txs.size()));
	for (const auto& tx : txs)
		buffer.write_raw(tx->serialize().get_buffer());

	return buffer;
}

bool Block::deserialize(BinaryBuffer& buffer)
{
	uint64_t new_version = 0;
	if (!buffer.read(new_version))
		return false;

	std::string new_prev_block_hash;
	if (!buffer.read(new_prev_block_hash))
		return false;

	std::string new_merkle_hash;
	if (!buffer.read(new_merkle_hash))
		return false;

	int64_t new_timestamp = -1;
	if (!buffer.read(new_timestamp))
		return false;

	uint8_t new_bits = 0;
	if (!buffer.read(new_bits))
		return false;

	uint64_t new_nonce = 0;
	if (!buffer.read(new_nonce))
		return false;

	uint32_t txs_size = 0;
	if (!buffer.read_size(txs_size))
		return false;

	std::vector<std::shared_ptr<Tx>> new_txs;
	new_txs.reserve(txs_size);
	for (uint32_t i = 0; i < txs_size; i++)
	{
		auto tx = std::make_shared<Tx>();
		if (!tx->deserialize(buffer))
			return false;
		new_txs.push_back(std::move(tx));
	}

	version = new_version;
	prev_block_hash = std::move(new_prev_block_hash);
	merkle_hash = std::move(new_merkle_hash);
	timestamp = new_timestamp;
	bits = new_bits;
	nonce = new_nonce;
	txs = std::move(new_txs);
	cached_id_.clear();

	return true;
}

bool Block::operator==(const Block& obj) const
{
	if (this == &obj)
	{
		return true;
	}

	if (tied() != obj.tied())
		return false;

	if (txs.size() != obj.txs.size())
		return false;
	for (uint32_t i = 0; i < txs.size(); i++)
	{
		if (*txs[i] != *obj.txs[i])
		{
			return false;
		}
	}

	return true;
}
