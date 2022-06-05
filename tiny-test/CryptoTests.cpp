#include "pch.hpp"

#include <string>

#include "../tiny-lib/Base58.hpp"
#include "../tiny-lib/ECDSA.hpp"
#include "../tiny-lib/RIPEMD160.hpp"
#include "../tiny-lib/SHA256.hpp"
#include "../tiny-lib/Utils.hpp"
#include "gtest/gtest.h"

TEST(CryptoTest, SHA256_Hashing)
{
	const auto hash = Utils::ByteArrayToHexString(SHA256::HashBinary(Utils::StringToByteArray("foo")));

	EXPECT_EQ(hash, "2c26b46b68ffc68ff99b453c1d30413413422d706483bfa0f98a5e886266e7ae");
}

TEST(CryptoTest, SHA256d_Hashing)
{
	const auto hash = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray("foo")));

	EXPECT_EQ(hash, "c7ade88fc7a21498a6a5e5c385e1f68bed822b72aa63c4a9a48a02c2466ee29e");
}

TEST(CryptoTest, RIPEMD160_Hashing)
{
	const auto hash = Utils::ByteArrayToHexString(RIPEMD160::HashBinary(Utils::StringToByteArray("foo")));

	EXPECT_EQ(hash, "42cfa211018ea492fdee45ac637b7972a0ad6873");
}

TEST(CryptoTest, Base58_Encode)
{
	const auto hash = Base58::Encode(Utils::StringToByteArray("foo"));

	EXPECT_EQ(hash, "bQbp");
}

TEST(CryptoTest, ECDSA_KeyPairGeneration)
{
	auto [priv_key, pub_key] = ECDSA::Generate();

	const auto priv_key_string = Utils::ByteArrayToHexString(priv_key);
	const auto pub_key_string = Utils::ByteArrayToHexString(pub_key);

	EXPECT_FALSE(priv_key_string.empty());
	EXPECT_FALSE(pub_key_string.empty());
}

TEST(CryptoTest, ECDSA_GetPubKeyFromPrivKey)
{
	const auto priv_key = Utils::HexStringToByteArray(
		"18e14a7b6a307f426a94f8114701e7c8e774e7f9a47e2c2035db29a206321725");
	const auto pub_key = Utils::ByteArrayToHexString(ECDSA::GetPubKeyFromPrivKey(priv_key));

	EXPECT_EQ(pub_key, "0250863ad64a87ae8a2fe83c1af1a8403cb53f53e486d8511dad8a04887e5b2352");
}

TEST(CryptoTest, ECDSA_GenerateKeyPairAndGetPubKeyFromPrivKey)
{
	auto [priv_key, pub_key] = ECDSA::Generate();

	const auto pub_key_string = Utils::ByteArrayToHexString(pub_key);

	const auto pub_key_string_from_priv_key = Utils::ByteArrayToHexString(ECDSA::GetPubKeyFromPrivKey(priv_key));

	EXPECT_FALSE(pub_key_string.empty());
	EXPECT_FALSE(pub_key_string_from_priv_key.empty());
	EXPECT_EQ(pub_key_string_from_priv_key, pub_key_string);
}

TEST(CryptoTest, ECDSA_SigningAndVerification)
{
	auto [priv_key, pub_key] = ECDSA::Generate();

	const std::string msg = "foo";

	const auto msg_arr = Utils::StringToByteArray(msg);
	const auto sig = ECDSA::SignMsg(msg_arr, priv_key);

	EXPECT_FALSE(sig.empty());
	EXPECT_TRUE(ECDSA::VerifySig(sig, msg_arr, pub_key));
}
