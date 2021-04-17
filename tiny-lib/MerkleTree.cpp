#include "pch.hpp"

#include <cstdint>

#include "MerkleTree.hpp"
#include "Utils.hpp"
#include "SHA256.hpp"

MerkleNode::MerkleNode(const std::string& value, const std::vector<std::string>& children)
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
		auto node = std::make_shared<MerkleNode>(l);

		nodes.push_back(node);
	}

	return FindRooot(nodes);
}

std::vector<std::vector<std::string>> MerkleTree::Chunk(const std::vector<std::string>& hashes, size_t chunkSize)
{
	std::vector<std::vector<std::string>> chunks;

	std::vector<std::string> chunk;
	chunk.reserve(chunkSize);
	for (const auto& hash : hashes)
	{
		chunk.push_back(hash);
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
	std::vector<std::string> hashes;

	for (const auto& node : nodes)
	{
		hashes.push_back(node->Value);
	}

	std::vector<std::shared_ptr<MerkleNode>> newLevel;
	auto chunks = Chunk(hashes, 2);
	for (const auto& chunk : chunks)
	{
		std::string combinedId = chunk[0] + chunk[1];
		std::vector<uint8_t> combinedId_vec(combinedId.begin(), combinedId.end());

		std::string combinedHash = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(combinedId_vec));

		auto node = std::make_shared<MerkleNode>(combinedHash, chunk);

		newLevel.push_back(node);
	}

	return newLevel.size() > 1 ? FindRooot(newLevel) : newLevel[0];
}
