#pragma once
#include <memory>
#include <string>
#include <vector>

#include "core/tx.hpp"

class MerkleNode
{
public:
	MerkleNode(std::string value, std::vector<std::shared_ptr<MerkleNode>> children);
	MerkleNode(std::string value);

	std::string value;
	std::vector<std::shared_ptr<MerkleNode>> children;
};

class MerkleTree
{
public:
	static std::shared_ptr<MerkleNode> get_root(const std::vector<std::string>& leaves);

	static std::shared_ptr<MerkleNode> get_root_of_txs(const std::vector<std::shared_ptr<Tx>>& txs);

private:
	static std::vector<std::vector<std::shared_ptr<MerkleNode>>> chunk(
		const std::vector<std::shared_ptr<MerkleNode>>& nodes, uint32_t chunk_size);

	static std::shared_ptr<MerkleNode> find_root(const std::vector<std::shared_ptr<MerkleNode>>& nodes);
};
