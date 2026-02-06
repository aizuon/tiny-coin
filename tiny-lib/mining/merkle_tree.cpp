#include "mining/merkle_tree.hpp"

#include "crypto/sha256.hpp"
#include "util/utils.hpp"

MerkleNode::MerkleNode(std::string value, std::vector<std::shared_ptr<MerkleNode>> children)
	: value(std::move(value)), children(std::move(children))
{}

MerkleNode::MerkleNode(std::string value)
	: value(std::move(value))
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
	std::vector<std::shared_ptr<MerkleNode>> nodes;
	nodes.reserve(txs.size());
	for (const auto& tx : txs)
	{
		nodes.push_back(std::make_shared<MerkleNode>(tx->id()));
	}
	return find_root(nodes);
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
	if (nodes.empty())
		return nullptr;

	if (nodes.size() == 1)
		return nodes.front();

	const auto chunks = chunk(nodes, 2);
	std::vector<std::shared_ptr<MerkleNode>> new_level;
	new_level.reserve(chunks.size());
	for (const auto& chunk : chunks)
	{
		auto combined_bytes = Utils::hex_string_to_byte_array(chunk[0]->value);
		auto right_bytes = Utils::hex_string_to_byte_array(chunk[1]->value);
		combined_bytes.insert(combined_bytes.end(), right_bytes.begin(), right_bytes.end());

		std::string combined_hash = Utils::byte_array_to_hex_string(
			SHA256::double_hash_binary(combined_bytes));

		auto node = std::make_shared<MerkleNode>(std::move(combined_hash), chunk);

		new_level.push_back(node);
	}

	return new_level.size() > 1 ? find_root(new_level) : new_level.front();
}
