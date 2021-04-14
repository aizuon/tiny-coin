#pragma once
#include <vector>
#include <string>
#include <memory>

class MerkleNode
{
public:
	std::string Value;
	std::vector<std::string> Children;
};

class MerkleTree
{
public:
	static std::shared_ptr<MerkleNode> GetRoot(const std::vector<std::string>& leaves);
};