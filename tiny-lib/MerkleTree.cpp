#include "pch.hpp"

#include <cstdint>

#include "MerkleTree.hpp"
#include "Utils.hpp"
#include "SHA256.hpp"

MerkleNode::MerkleNode(const std::string& value, const std::vector<std::shared_ptr<MerkleNode>>& children)
	: Value(value), Children(children)
{

}

MerkleNode::MerkleNode(const std::string& value)
	: Value(value)
{

}

std::shared_ptr<MerkleNode> MerkleTree::GetRoot(std::vector<std::string> leaves)
{
	if (leaves.size() % 2 == 1)
		leaves.push_back(leaves.back());

	std::vector<std::shared_ptr<MerkleNode>> nodes;
	for (const auto& l : leaves)
	{
		auto node = std::make_shared<MerkleNode>(Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(l))));

		nodes.push_back(node);
	}

	return FindRooot(nodes);
}

std::shared_ptr<MerkleNode> MerkleTree::GetRootOfTxs(const std::vector<std::shared_ptr<Tx>>& txs)
{
	std::vector<std::string> hashes;
	hashes.reserve(txs.size());
	for (const auto& tx : txs)
	{
		hashes.emplace_back(tx->Id());
	}
	return MerkleTree::GetRoot(hashes);
}

std::vector<std::vector<std::shared_ptr<MerkleNode>>> MerkleTree::Chunk(const std::vector<std::shared_ptr<MerkleNode>>& nodes, size_t chunkSize)
{
	std::vector<std::vector<std::shared_ptr<MerkleNode>>> chunks;

	std::vector<std::shared_ptr<MerkleNode>> chunk;
	chunk.reserve(chunkSize);
	for (const auto& node : nodes)
	{
		chunk.push_back(node);
		if (chunk.size() == chunkSize)
		{
			chunks.push_back(chunk);
			chunk.clear();
		}
	}

	return chunks;
}

std::shared_ptr<MerkleNode> MerkleTree::FindRooot(const std::vector<std::shared_ptr<MerkleNode>>& nodes)
{
	std::vector<std::shared_ptr<MerkleNode>> newLevel;
	auto chunks = Chunk(nodes, 2);
	for (const auto& chunk : chunks)
	{
		std::string combinedId = chunk[0]->Value + chunk[1]->Value;

		std::string combinedHash = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(combinedId)));

		auto node = std::make_shared<MerkleNode>(combinedHash, chunk);

		newLevel.push_back(node);
	}

	return newLevel.size() > 1 ? FindRooot(newLevel) : newLevel[0];
}
