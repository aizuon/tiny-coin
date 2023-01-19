#include "pch.hpp"
#include "MerkleTree.hpp"

#include "SHA256.hpp"
#include "Utils.hpp"

MerkleNode::MerkleNode(const std::string& value, const std::vector<std::shared_ptr<MerkleNode>>& children)
	: Value(value), Children(children)
{
}

MerkleNode::MerkleNode(const std::string& value)
	: Value(value)
{
}

std::shared_ptr<MerkleNode> MerkleTree::GetRoot(const std::vector<std::string>& leaves)
{
	std::vector<std::shared_ptr<MerkleNode>> nodes;
	for (const auto& l : leaves)
	{
		auto node = std::make_shared<MerkleNode>(
			Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(l))));

		nodes.push_back(node);
	}

	return FindRoot(nodes);
}

std::shared_ptr<MerkleNode> MerkleTree::GetRootOfTxs(const std::vector<std::shared_ptr<Tx>>& txs)
{
	std::vector<std::string> hashes;
	hashes.reserve(txs.size());
	for (const auto& tx : txs)
	{
		hashes.emplace_back(tx->Id());
	}
	return GetRoot(hashes);
}

std::vector<std::vector<std::shared_ptr<MerkleNode>>> MerkleTree::Chunk(
	const std::vector<std::shared_ptr<MerkleNode>>& nodes, uint32_t chunk_size)
{
	std::vector<std::vector<std::shared_ptr<MerkleNode>>> chunks;

	std::vector<std::shared_ptr<MerkleNode>> chunk;
	chunk.reserve(chunk_size);
	for (const auto& node : nodes)
	{
		chunk.push_back(node);
		if (chunk.size() == chunk_size)
		{
			chunks.push_back(chunk);
			chunk.clear();
		}
	}
	if (!chunk.empty())
	{
		while (chunk.size() != chunk_size)
		{
			chunk.push_back(chunk.back());
		}
		chunks.push_back(chunk);
	}

	return chunks;
}

std::shared_ptr<MerkleNode> MerkleTree::FindRoot(const std::vector<std::shared_ptr<MerkleNode>>& nodes)
{
	std::vector<std::shared_ptr<MerkleNode>> new_level;
	const auto chunks = Chunk(nodes, 2);
	for (const auto& chunk : chunks)
	{
		std::string combined_id;
		for (const auto& node : chunk)
		{
			combined_id += node->Value;
		}

		std::string combined_hash = Utils::ByteArrayToHexString(
			SHA256::DoubleHashBinary(Utils::StringToByteArray(combined_id)));

		auto node = std::make_shared<MerkleNode>(combined_hash, chunk);

		new_level.push_back(node);
	}

	return new_level.size() > 1 ? FindRoot(new_level) : new_level[0];
}
