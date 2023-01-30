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

	const auto combined_h = Utils::ByteArrayToHexString(
		SHA256::DoubleHashBinary(Utils::StringToByteArray(foo_h + bar_h)));
	EXPECT_EQ(combined_h, root->Value);
	EXPECT_EQ(foo_h, root->Children[0]->Value);
	EXPECT_EQ(bar_h, root->Children[1]->Value);
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

	EXPECT_EQ(2, root->Children.size());
	auto combined_h1 = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(foo_h + bar_h)));
	auto combined_h2 = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(baz_h + baz_h)));
	EXPECT_EQ(combined_h1, root->Children[0]->Value);
	EXPECT_EQ(combined_h2, root->Children[1]->Value);
}
