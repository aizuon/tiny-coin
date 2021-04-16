#include "pch.hpp"

#include "gtest/gtest.h"

#include "../tiny-lib/ECDSA.hpp"
#include "../tiny-lib/SHA256.hpp"
#include "../tiny-lib/RIPEMD160.hpp"
#include "../tiny-lib/Base58.hpp"
#include "../tiny-lib/Utils.hpp"

TEST(CryptoTest, SHA256_Hashing)
{
	auto hash = Utils::ByteArrayToHexString(SHA256::HashBinary(Utils::StringToByteArray("this is a test string")));

	EXPECT_EQ(hash, "f6774519d1c7a3389ef327e9c04766b999db8cdfb85d1346c471ee86d65885bc");
}

TEST(CryptoTest, SHA256d_Hashing)
{
	auto hash = Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray("this is a test string")));

	EXPECT_EQ(hash, "ed487d58f41720ec58e798ebcd699ec7149f0f9c90868edb0748d7e4bdd8d134");
}

TEST(CryptoTest, RIPEMD160_Hashing)
{
	auto hash = Utils::ByteArrayToHexString(RIPEMD160::HashBinary(Utils::StringToByteArray("this is a test string")));

	EXPECT_EQ(hash, "5991f4fcb91b7eab340ba5301edabf7b8f8926e8");
}

TEST(CryptoTest, Base58_Encode)
{
	auto hash = Base58::Encode(Utils::StringToByteArray("this is a test string"));

	EXPECT_EQ(hash, "8AArJ45Yvcrsr4CB6zUCEPx9NKDyg");
}

TEST(CryptoTest, ECDSA_KeyPairGeneration)
{
	auto [priv_key, pub_key] = ECDSA::Generate();

	std::string priv_key_string = Utils::ByteArrayToHexString(priv_key);
	std::string pub_key_string = Utils::ByteArrayToHexString(pub_key);

	EXPECT_FALSE(priv_key_string.empty());
	EXPECT_FALSE(pub_key_string.empty());
}

TEST(CryptoTest, ECDSA_GetPubKeyFromPrivKey)
{
	auto priv_key = Utils::HexStringToByteArray("18e14a7b6a307f426a94f8114701e7c8e774e7f9a47e2c2035db29a206321725");
	auto pub_key = Utils::ByteArrayToHexString(ECDSA::GetPubKeyFromPrivKey(priv_key));

	EXPECT_EQ(pub_key, "0250863ad64a87ae8a2fe83c1af1a8403cb53f53e486d8511dad8a04887e5b2352");
}

TEST(CryptoTest, ECDSA_GenerateKeyPairAndGetPubKeyFromPrivKey)
{
	auto [priv_key, pub_key] = ECDSA::Generate();

	std::string pub_key_string = Utils::ByteArrayToHexString(pub_key);

	auto pub_key_string_from_priv_key = Utils::ByteArrayToHexString(ECDSA::GetPubKeyFromPrivKey(priv_key));

	EXPECT_EQ(pub_key_string_from_priv_key, pub_key_string);
}