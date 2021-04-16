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
}