#include <memory>
#include <string>
#include <vector>

#include "mining/merkle_tree.hpp"
#include "crypto/sha256.hpp"
#include "core/tx.hpp"
#include "core/tx_in.hpp"
#include "core/tx_out.hpp"
#include "util/utils.hpp"
#include <gtest/gtest.h>

TEST(MerkleTreeTest, OneChain)
{
	const std::string foo = "foo";
	const std::string bar = "bar";
	const std::vector tree{ foo, bar };

	const auto root = MerkleTree::get_root(tree);
	auto foo_bytes = SHA256::double_hash_binary(Utils::string_to_byte_array(foo));
	auto bar_bytes = SHA256::double_hash_binary(Utils::string_to_byte_array(bar));
	const auto foo_h = Utils::byte_array_to_hex_string(foo_bytes);
	const auto bar_h = Utils::byte_array_to_hex_string(bar_bytes);

	auto combined_bytes = foo_bytes;
	combined_bytes.insert(combined_bytes.end(), bar_bytes.begin(), bar_bytes.end());
	const auto combined_h = Utils::byte_array_to_hex_string(
		SHA256::double_hash_binary(combined_bytes));
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
	auto foo_bytes = SHA256::double_hash_binary(Utils::string_to_byte_array(foo));
	auto bar_bytes = SHA256::double_hash_binary(Utils::string_to_byte_array(bar));
	auto baz_bytes = SHA256::double_hash_binary(Utils::string_to_byte_array(baz));
	const auto foo_h = Utils::byte_array_to_hex_string(foo_bytes);
	const auto bar_h = Utils::byte_array_to_hex_string(bar_bytes);
	const auto baz_h = Utils::byte_array_to_hex_string(baz_bytes);

	EXPECT_EQ(2, root->children.size());
	auto combined1 = foo_bytes;
	combined1.insert(combined1.end(), bar_bytes.begin(), bar_bytes.end());
	auto combined_h1 = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(combined1));
	auto combined2 = baz_bytes;
	combined2.insert(combined2.end(), baz_bytes.begin(), baz_bytes.end());
	auto combined_h2 = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(combined2));
	EXPECT_EQ(combined_h1, root->children[0]->value);
	EXPECT_EQ(combined_h2, root->children[1]->value);
}

TEST(MerkleTreeTest, SingleLeaf)
{
	const std::vector<std::string> leaves{ "only" };
	auto root = MerkleTree::get_root(leaves);

	ASSERT_NE(nullptr, root);
	auto expected = Utils::byte_array_to_hex_string(
		SHA256::double_hash_binary(Utils::string_to_byte_array("only")));
	EXPECT_EQ(expected, root->value);
	EXPECT_TRUE(root->children.empty());
}

TEST(MerkleTreeTest, EmptyInput)
{
	const std::vector<std::string> leaves{};
	auto root = MerkleTree::get_root(leaves);
	EXPECT_EQ(nullptr, root);
}

TEST(MerkleTreeTest, FourLeavesPowerOfTwo)
{
	const std::vector<std::string> leaves{ "a", "b", "c", "d" };
	auto root = MerkleTree::get_root(leaves);

	ASSERT_NE(nullptr, root);
	ASSERT_EQ(2, root->children.size());

	auto a_h = SHA256::double_hash_binary(Utils::string_to_byte_array("a"));
	auto b_h = SHA256::double_hash_binary(Utils::string_to_byte_array("b"));
	auto c_h = SHA256::double_hash_binary(Utils::string_to_byte_array("c"));
	auto d_h = SHA256::double_hash_binary(Utils::string_to_byte_array("d"));

	auto ab = a_h;
	ab.insert(ab.end(), b_h.begin(), b_h.end());
	auto ab_hash = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(ab));

	auto cd = c_h;
	cd.insert(cd.end(), d_h.begin(), d_h.end());
	auto cd_hash = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(cd));

	EXPECT_EQ(ab_hash, root->children[0]->value);
	EXPECT_EQ(cd_hash, root->children[1]->value);

	auto ab_bytes = Utils::hex_string_to_byte_array(ab_hash);
	auto cd_bytes = Utils::hex_string_to_byte_array(cd_hash);
	ab_bytes.insert(ab_bytes.end(), cd_bytes.begin(), cd_bytes.end());
	auto expected_root = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(ab_bytes));
	EXPECT_EQ(expected_root, root->value);
}

TEST(MerkleTreeTest, GetRootOfTxsConsistentWithGetRoot)
{
	auto tx_in = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{0x01}, std::vector<uint8_t>{}, -1);
	auto tx_out = std::make_shared<TxOut>(100, "addr");
	auto tx1 = std::make_shared<Tx>(std::vector{ tx_in }, std::vector{ tx_out }, 0);

	auto tx_in2 = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{0x02}, std::vector<uint8_t>{}, -1);
	auto tx2 = std::make_shared<Tx>(std::vector{ tx_in2 }, std::vector{ tx_out }, 0);

	auto root = MerkleTree::get_root_of_txs(std::vector{ tx1, tx2 });
	ASSERT_NE(nullptr, root);

	auto left = Utils::hex_string_to_byte_array(tx1->id());
	auto right = Utils::hex_string_to_byte_array(tx2->id());
	left.insert(left.end(), right.begin(), right.end());
	auto expected = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(left));
	EXPECT_EQ(expected, root->value);
}
