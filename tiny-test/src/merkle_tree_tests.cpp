#include <string>
#include <vector>

#include "mining/merkle_tree.hpp"
#include "crypto/sha256.hpp"
#include "util/utils.hpp"
#include <gtest/gtest.h>

TEST(MerkleTreeTest, OneChain)
{
	const std::string foo = "foo";
	const std::string bar = "bar";
	const std::vector tree{ foo, bar };

	const auto root = MerkleTree::get_root(tree);
	const auto foo_h = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(Utils::string_to_byte_array(foo)));
	const auto bar_h = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(Utils::string_to_byte_array(bar)));

	const auto combined_h = Utils::byte_array_to_hex_string(
		SHA256::double_hash_binary(Utils::string_to_byte_array(foo_h + bar_h)));
	EXPECT_EQ(combined_h, root->value);
	EXPECT_EQ(foo_h, root->children[0]->value);
	EXPECT_EQ(bar_h, root->children[1]->value);
}

TEST(MerkleTreeTest, TwoChain)
{
	std::string foo = "foo";
	std::string bar = "bar";
	std::string baz = "baz";

	std::vector tree{ foo, bar, baz };

	auto root = MerkleTree::get_root(tree);
	auto foo_h = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(Utils::string_to_byte_array(foo)));
	auto bar_h = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(Utils::string_to_byte_array(bar)));
	auto baz_h = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(Utils::string_to_byte_array(baz)));

	EXPECT_EQ(2, root->children.size());
	auto combined_h1 = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(Utils::string_to_byte_array(foo_h + bar_h)));
	auto combined_h2 = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(Utils::string_to_byte_array(baz_h + baz_h)));
	EXPECT_EQ(combined_h1, root->children[0]->value);
	EXPECT_EQ(combined_h2, root->children[1]->value);
}
