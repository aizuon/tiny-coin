#include <cstdint>
#include <string>
#include <vector>

#include "crypto/base58.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/hmac_sha512.hpp"
#include "crypto/ripemd160.hpp"
#include "crypto/sha256.hpp"
#include "crypto/sig_cache.hpp"
#include "util/utils.hpp"
#include <gtest/gtest.h>

TEST(CryptoTest, SHA256_Hashing)
{
	const auto hash = Utils::byte_array_to_hex_string(SHA256::hash_binary(Utils::string_to_byte_array("foo")));

	EXPECT_EQ("2c26b46b68ffc68ff99b453c1d30413413422d706483bfa0f98a5e886266e7ae", hash);
}

TEST(CryptoTest, SHA256d_Hashing)
{
	const auto hash = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(Utils::string_to_byte_array("foo")));

	EXPECT_EQ("c7ade88fc7a21498a6a5e5c385e1f68bed822b72aa63c4a9a48a02c2466ee29e", hash);
}

TEST(CryptoTest, RIPEMD160_Hashing)
{
	const auto hash = Utils::byte_array_to_hex_string(RIPEMD160::hash_binary(Utils::string_to_byte_array("foo")));

	EXPECT_EQ("42cfa211018ea492fdee45ac637b7972a0ad6873", hash);
}

TEST(CryptoTest, Base58_Encode)
{
	const auto hash = Base58::encode(Utils::string_to_byte_array("foo"));

	EXPECT_EQ("bQbp", hash);
}

TEST(CryptoTest, ECDSA_KeyPairGeneration)
{
	auto [priv_key, pub_key] = ECDSA::generate();

	const auto priv_key_string = Utils::byte_array_to_hex_string(priv_key);
	const auto pub_key_string = Utils::byte_array_to_hex_string(pub_key);

	EXPECT_FALSE(priv_key_string.empty());
	EXPECT_FALSE(pub_key_string.empty());
}

TEST(CryptoTest, ECDSA_GetPubKeyFromPrivKey)
{
	const auto priv_key = Utils::hex_string_to_byte_array(
		"18e14a7b6a307f426a94f8114701e7c8e774e7f9a47e2c2035db29a206321725");
	const auto pub_key = Utils::byte_array_to_hex_string(ECDSA::get_pub_key_from_priv_key(priv_key));

	EXPECT_EQ("0250863ad64a87ae8a2fe83c1af1a8403cb53f53e486d8511dad8a04887e5b2352", pub_key);
}

TEST(CryptoTest, ECDSA_GenerateKeyPairAndGetPubKeyFromPrivKey)
{
	auto [priv_key, pub_key] = ECDSA::generate();

	const auto pub_key_string = Utils::byte_array_to_hex_string(pub_key);

	const auto pub_key_string_from_priv_key = Utils::byte_array_to_hex_string(ECDSA::get_pub_key_from_priv_key(priv_key));

	EXPECT_FALSE(pub_key_string.empty());
	EXPECT_FALSE(pub_key_string_from_priv_key.empty());
	EXPECT_EQ(pub_key_string, pub_key_string_from_priv_key);
}

TEST(CryptoTest, ECDSA_SigningAndVerification)
{
	auto [priv_key, pub_key] = ECDSA::generate();

	const std::string msg = "foo";

	const auto msg_arr = Utils::string_to_byte_array(msg);
	const auto sig = ECDSA::sign_msg(msg_arr, priv_key);

	EXPECT_FALSE(sig.empty());
	EXPECT_TRUE(ECDSA::verify_sig(sig, msg_arr, pub_key));
}

TEST(CryptoTest, HMACSHA512_BasicVector)
{
	const std::string key_str = "Jefe";
	const std::vector<uint8_t> key(key_str.begin(), key_str.end());
	const std::string data_str = "what do ya want for nothing?";
	const std::vector<uint8_t> data(data_str.begin(), data_str.end());

	const auto hex = Utils::byte_array_to_hex_string(HMACSHA512::hash(key, data));

	EXPECT_EQ(
		"164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea250554"
		"9758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737",
		hex);
}

TEST(CryptoTest, SHA256_EmptyInput)
{
	auto hash = Utils::byte_array_to_hex_string(SHA256::hash_binary({}));
	EXPECT_EQ("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", hash);
}

TEST(CryptoTest, SHA256d_EmptyInput)
{
	auto hash = Utils::byte_array_to_hex_string(SHA256::double_hash_binary({}));
	auto inner = SHA256::hash_binary({});
	auto expected = Utils::byte_array_to_hex_string(SHA256::hash_binary(inner));
	EXPECT_EQ(expected, hash);
}

TEST(CryptoTest, RIPEMD160_EmptyInput)
{
	auto hash = Utils::byte_array_to_hex_string(RIPEMD160::hash_binary({}));
	EXPECT_EQ("9c1185a5c5e9fc54612808977ee8f548b2258d31", hash);
}

TEST(CryptoTest, Base58_LeadingZeroBytes)
{
	std::vector<uint8_t> input{ 0x00, 0x00, 0x01 };
	auto encoded = Base58::encode(input);

	ASSERT_GE(encoded.size(), 2);
	EXPECT_EQ('1', encoded[0]);
	EXPECT_EQ('1', encoded[1]);
}

class SigCacheTest : public ::testing::Test
{
protected:
	void SetUp() override { SigCache::clear(); }
	void TearDown() override { SigCache::clear(); }
};

TEST_F(SigCacheTest, AddAndContains)
{
	const std::vector<uint8_t> sig{ 0x30, 0x44, 0x02, 0x20 };
	const std::vector<uint8_t> msg{ 0xDE, 0xAD, 0xBE, 0xEF };
	const std::vector<uint8_t> pub{ 0x04, 0x01, 0x02, 0x03 };

	EXPECT_FALSE(SigCache::contains(sig, msg, pub));

	SigCache::add(sig, msg, pub);
	EXPECT_TRUE(SigCache::contains(sig, msg, pub));
}

TEST_F(SigCacheTest, DifferentInputsNotFound)
{
	const std::vector<uint8_t> sig{ 0x01 };
	const std::vector<uint8_t> msg{ 0x02 };
	const std::vector<uint8_t> pub{ 0x03 };

	SigCache::add(sig, msg, pub);

	EXPECT_FALSE(SigCache::contains({ 0xFF }, msg, pub));
	EXPECT_FALSE(SigCache::contains(sig, { 0xFF }, pub));
	EXPECT_FALSE(SigCache::contains(sig, msg, { 0xFF }));
}

TEST_F(SigCacheTest, ClearRemovesEntries)
{
	const std::vector<uint8_t> sig{ 0x01 };
	const std::vector<uint8_t> msg{ 0x02 };
	const std::vector<uint8_t> pub{ 0x03 };

	SigCache::add(sig, msg, pub);
	EXPECT_TRUE(SigCache::contains(sig, msg, pub));

	SigCache::clear();
	EXPECT_FALSE(SigCache::contains(sig, msg, pub));
}
