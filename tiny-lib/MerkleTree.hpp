#pragma once
#include <vector>
#include <string>
#include <memory>

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

private:
	static std::vector<std::vector<std::shared_ptr<MerkleNode>>> Chunk(const std::vector<std::shared_ptr<MerkleNode>>& nodes, size_t chunkSize);

	static std::shared_ptr<MerkleNode> FindRooot(const std::vector<std::shared_ptr<MerkleNode>>& nodes);
};