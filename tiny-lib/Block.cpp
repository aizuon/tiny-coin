#include "pch.hpp"

#include <fmt/format.h>

#include "Block.hpp"
#include "Utils.hpp"
#include "SHA256.hpp"

Block::Block(uint64_t version, const std::string& prevBlockHash, const std::string& markleHash, int64_t timestamp, uint8_t bits, uint64_t nonce,
	const std::vector<std::shared_ptr<Tx>>& txs)
	: Version(version), PrevBlockHash(prevBlockHash), MerkleHash(markleHash), Timestamp(timestamp), Bits(bits), Nonce(nonce), Txs(txs)
{

}

std::string Block::Header(uint64_t nonce /*= 0*/) const
{
	uint64_t used_nonce = nonce == 0 ? Nonce : nonce;

	return fmt::format("{}{}{}{}{}{}", Version, PrevBlockHash, MerkleHash, Timestamp, Bits, used_nonce);
}

std::string Block::Id() const
{
	return Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(Header())));
}

BinaryBuffer Block::Serialize() const
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

	size_t txsSize = 0;
	if (!buffer.Read(txsSize))
	{
		*this = std::move(copy);

		return false;
	}
	Txs = std::vector<std::shared_ptr<Tx>>();
	Txs.reserve(txsSize);
	for (size_t i = 0; i < txsSize; i++)
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
	for (size_t i = 0; i < Txs.size(); i++)
	{
		if (*Txs[i] != *obj.Txs[i])
		{
			return false;
		}
	}

	return true;
}
