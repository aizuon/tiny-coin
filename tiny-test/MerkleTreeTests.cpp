#include "pch.hpp"

#include <string>
#include <vector>

#include "../tiny-lib/MerkleTree.hpp"
#include "../tiny-lib/SHA256.hpp"
#include "../tiny-lib/Utils.hpp"
#include "gtest/gtest.h"

TEST(MerkleTreeTest, OneChain)
{
	const std::string foo = "foo";
	const std::string bar = "bar";
	const std::vector tree{ foo, bar };

	const auto root = MerkleTree::GetRoot(tree);
	const auto foo_h = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(foo)));
	const auto bar_h = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(bar)));

	EXPECT_TRUE(root != nullptr);
	const auto combined_h = Utils::ByteArrayToHexString(
		SHA256::DoubleHashBinary(Utils::StringToByteArray(foo_h + bar_h)));
	EXPECT_TRUE(root->Value == combined_h);
	EXPECT_TRUE(root->Children[0]->Value == foo_h);
	EXPECT_TRUE(root->Children[1]->Value == bar_h);
}

TEST(MerkleTreeTest, TwoChain)
{
	std::string foo = "foo";
	std::string bar = "bar";
	std::string baz = "baz";

	std::vector tree{ foo, bar, baz };

	auto root = MerkleTree::GetRoot(tree);
	auto foo_h = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(foo)));
	auto bar_h = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(bar)));
	auto baz_h = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(baz)));

	EXPECT_TRUE(root != nullptr);
	EXPECT_TRUE(root->Children.size() == 2);
	auto combined_h1 = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(foo_h + bar_h)));
	auto combined_h2 = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(baz_h + baz_h)));
	EXPECT_TRUE(root->Children[0]->Value == combined_h1);
	EXPECT_TRUE(root->Children[1]->Value == combined_h2);
}
