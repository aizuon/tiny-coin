#include "core/block.hpp"

#include "crypto/sha256.hpp"
#include "util/utils.hpp"

Block::Block(uint64_t version, const std::string& prev_block_hash, const std::string& merkle_hash, int64_t timestamp,
	uint8_t bits, uint64_t nonce,
	const std::vector<std::shared_ptr<Tx>>& txs)
	: version(version), prev_block_hash(prev_block_hash), merkle_hash(merkle_hash), timestamp(timestamp), bits(bits),
	nonce(nonce), txs(txs)
{}

BinaryBuffer Block::header(std::optional<uint64_t> nonce /*= std::nullopt*/) const
{
	BinaryBuffer buffer;

	buffer.write(version);

	buffer.write(prev_block_hash);
	buffer.write(merkle_hash);

	buffer.write(timestamp);

	buffer.write(bits);

	buffer.write(nonce.value_or(this->nonce));

	return buffer;
}

std::string Block::id() const
{
	return Utils::byte_array_to_hex_string(SHA256::double_hash_binary(header().get_buffer()));
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
	auto copy = *this;

	if (!buffer.read(version))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.read(prev_block_hash))
	{
		*this = std::move(copy);

		return false;
	}
	if (!buffer.read(merkle_hash))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.read(timestamp))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.read(bits))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.read(nonce))
	{
		*this = std::move(copy);

		return false;
	}

	uint32_t txs_size = 0;
	if (!buffer.read_size(txs_size))
	{
		*this = std::move(copy);

		return false;
	}
	txs.clear();
	txs.reserve(txs_size);
	for (uint32_t i = 0; i < txs_size; i++)
	{
		auto tx = std::make_shared<Tx>();
		if (!tx->deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		txs.push_back(tx);
	}

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
