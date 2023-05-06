#include "pch.hpp"
#include "Block.hpp"

#include "SHA256.hpp"
#include "Utils.hpp"

Block::Block(uint64_t version, const std::string& prev_block_hash, const std::string& merkle_hash, int64_t timestamp,
             uint8_t bits, uint64_t nonce,
             const std::vector<std::shared_ptr<Tx>>& txs)
	: Version(version), PrevBlockHash(prev_block_hash), MerkleHash(merkle_hash), Timestamp(timestamp), Bits(bits),
	  Nonce(nonce), Txs(txs)
{
}

BinaryBuffer Block::Header(uint64_t nonce /*= 0*/) const
{
	BinaryBuffer buffer;

	buffer.Write(Version);

	buffer.Write(PrevBlockHash);
	buffer.Write(MerkleHash);

	buffer.Write(Timestamp);

	buffer.Write(Bits);

	buffer.Write(nonce == 0 ? Nonce : nonce);

	return buffer;
}

std::string Block::Id() const
{
	return Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Header().GetBuffer()));
}

BinaryBuffer Block::Serialize() const
{
	BinaryBuffer buffer = Header();

	buffer.WriteSize(Txs.size());
	for (const auto& tx : Txs)
		buffer.WriteRaw(tx->Serialize().GetBuffer());

	return buffer;
}

bool Block::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	if (!buffer.Read(Version))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.Read(PrevBlockHash))
	{
		*this = std::move(copy);

		return false;
	}
	if (!buffer.Read(MerkleHash))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.Read(Timestamp))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.Read(Bits))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.Read(Nonce))
	{
		*this = std::move(copy);

		return false;
	}

	uint32_t txs_size = 0;
	if (!buffer.ReadSize(txs_size))
	{
		*this = std::move(copy);

		return false;
	}
	if (!Txs.empty())
		Txs.clear();
	Txs.reserve(txs_size);
	for (uint32_t i = 0; i < txs_size; i++)
	{
		auto tx = std::make_shared<Tx>();
		if (!tx->Deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		Txs.push_back(tx);
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

	if (Txs.size() != obj.Txs.size())
		return false;
	for (uint32_t i = 0; i < Txs.size(); i++)
	{
		if (*Txs[i] != *obj.Txs[i])
		{
			return false;
		}
	}

	return true;
}
