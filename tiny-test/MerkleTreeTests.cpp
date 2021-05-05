#include "pch.hpp"

#include <vector>
#include <string>
#include "gtest/gtest.h"

#include "../tiny-lib/Utils.hpp"
#include "../tiny-lib/MerkleTree.hpp"
#include "../tiny-lib/SHA256.hpp"

TEST(MerkleTreeTest, OneChain)
{
	std::string foo = "foo";
	std::string bar = "bar";
	std::vector tree{foo, bar};

	auto root = MerkleTree::GetRoot(tree);
	auto fooh = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(foo)));
	auto barh = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(bar)));

	EXPECT_TRUE(root != nullptr);
	auto combinedh = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(fooh + barh)));
	EXPECT_TRUE(root->Value == combinedh);
	EXPECT_TRUE(root->Children[0]->Value == fooh);
	EXPECT_TRUE(root->Children[1]->Value == barh);
}

TEST(MerkleTreeTest, TwoChain)
{
	std::string foo = "foo";
	std::string bar = "bar";
	std::string baz = "baz";

	std::vector tree{foo, bar, baz};

	auto root = MerkleTree::GetRoot(tree);
	auto fooh = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(foo)));
	auto barh = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(bar)));
	auto bazh = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(baz)));

	EXPECT_TRUE(root != nullptr);
	EXPECT_TRUE(root->Children.size() == 2);
	auto combinedh1 = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(fooh + barh)));
	auto combinedh2 = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(bazh + bazh)));
	EXPECT_TRUE(root->Children[0]->Value == combinedh1);
	EXPECT_TRUE(root->Children[1]->Value == combinedh2);
}
