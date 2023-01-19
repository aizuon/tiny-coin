#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Tx.hpp"

#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")

class MerkleNode
{
public:
	MerkleNode(const std::string& value, const std::vector<std::shared_ptr<MerkleNode>>& children);
	MerkleNode(const std::string& value);

	std::string Value;
	std::vector<std::shared_ptr<MerkleNode>> Children;
};

class MerkleTree
{
public:
	static std::shared_ptr<MerkleNode> GetRoot(std::vector<std::string> leaves);

	static std::shared_ptr<MerkleNode> GetRootOfTxs(const std::vector<std::shared_ptr<Tx>>& txs);

private:
	static std::vector<std::vector<std::shared_ptr<MerkleNode>>> Chunk(
		const std::vector<std::shared_ptr<MerkleNode>>& nodes, uint32_t chunk_size);

	static std::shared_ptr<MerkleNode> FindRoot(const std::vector<std::shared_ptr<MerkleNode>>& nodes);
};
