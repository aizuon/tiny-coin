#include "mining/merkle_tree.hpp"

#include "crypto/sha256.hpp"
#include "util/utils.hpp"

MerkleNode::MerkleNode(const std::string& value, const std::vector<std::shared_ptr<MerkleNode>>& children)
	: value(value), children(children)
{}

MerkleNode::MerkleNode(const std::string& value)
	: value(value)
{}

std::shared_ptr<MerkleNode> MerkleTree::get_root(const std::vector<std::string>& leaves)
{
	std::vector<std::shared_ptr<MerkleNode>> nodes;
	nodes.reserve(leaves.size());
	for (const auto& l : leaves)
	{
		auto node = std::make_shared<MerkleNode>(
			Utils::byte_array_to_hex_string(SHA256::double_hash_binary(Utils::string_to_byte_array(l))));

		nodes.push_back(node);
	}

	return find_root(nodes);
}

std::shared_ptr<MerkleNode> MerkleTree::get_root_of_txs(const std::vector<std::shared_ptr<Tx>>& txs)
{
	std::vector<std::string> hashes;
	hashes.reserve(txs.size());
	for (const auto& tx : txs)
	{
		hashes.emplace_back(tx->id());
	}
	return get_root(hashes);
}

std::vector<std::vector<std::shared_ptr<MerkleNode>>> MerkleTree::chunk(
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
			chunks.push_back(std::move(chunk));
			chunk.clear();
			chunk.reserve(chunk_size);
		}
	}
	if (!chunk.empty())
	{
		while (chunk.size() != chunk_size)
		{
			chunk.push_back(chunk.back());
		}
		chunks.push_back(std::move(chunk));
	}

	return chunks;
}

std::shared_ptr<MerkleNode> MerkleTree::find_root(const std::vector<std::shared_ptr<MerkleNode>>& nodes)
{
	const auto chunks = chunk(nodes, 2);
	std::vector<std::shared_ptr<MerkleNode>> new_level;
	new_level.reserve(chunks.size());
	for (const auto& chunk : chunks)
	{
		std::string combined_id;
		for (const auto& node : chunk)
		{
			combined_id += node->value;
		}

		std::string combined_hash = Utils::byte_array_to_hex_string(
			SHA256::double_hash_binary(Utils::string_to_byte_array(combined_id)));

		auto node = std::make_shared<MerkleNode>(combined_hash, chunk);

		new_level.push_back(node);
	}

	return new_level.size() > 1 ? find_root(new_level) : new_level[0];
}
