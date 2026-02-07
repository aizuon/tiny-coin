#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "crypto/ecdsa.hpp"
#include "util/utils.hpp"
#include "wallet/wallet.hpp"
#include <gtest/gtest.h>

TEST(WalletTest, PubKeyToAddress_TestVectors)
{
	{
		const auto pub_key = Utils::hex_string_to_byte_array(
			"0250863ad64a87ae8a2fe83c1af1a8403cb53f53e486d8511dad8a04887e5b2352");

		EXPECT_EQ("1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs",
			Wallet::pub_key_to_address(pub_key));
	}

	{
		const auto priv_key = Utils::hex_string_to_byte_array(
			"18e14a7b6a307f426a94f8114701e7c8e774e7f9a47e2c2035db29a206321725");
		const auto pub_key = ECDSA::get_pub_key_from_priv_key(priv_key);

		EXPECT_EQ("1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs",
			Wallet::pub_key_to_address(pub_key));
	}
}

TEST(WalletTest, AddressDerivationAndKeys)
{
	auto [priv_key, pub_key] = ECDSA::generate();

	const auto addr1 = Wallet::pub_key_to_address(pub_key);
	const auto addr2 = Wallet::pub_key_to_address(pub_key);
	EXPECT_EQ(addr1, addr2);

	const auto derived_pub = ECDSA::get_pub_key_from_priv_key(priv_key);
	EXPECT_EQ(pub_key, derived_pub);

	EXPECT_EQ(addr1, Wallet::pub_key_to_address(derived_pub));

	auto [priv2, pub2] = ECDSA::generate();
	EXPECT_NE(addr1, Wallet::pub_key_to_address(pub2));
}

TEST(WalletTest, InitWallet_CreatesNewWallet)
{
	const std::string path = "test_wallet_init.dat";
	std::remove(path.c_str());

	const auto [priv_key, pub_key, address] = Wallet::init_wallet(path);

	EXPECT_FALSE(priv_key.empty());
	EXPECT_FALSE(pub_key.empty());
	EXPECT_FALSE(address.empty());

	EXPECT_EQ(pub_key, ECDSA::get_pub_key_from_priv_key(priv_key));

	EXPECT_EQ(address, Wallet::pub_key_to_address(pub_key));

	std::remove(path.c_str());
}

TEST(WalletTest, SaveLoadRoundTrip)
{
	const std::string path = "test_wallet_reload.dat";
	std::remove(path.c_str());

	const auto [priv_key1, pub_key1, address1] = Wallet::init_wallet(path);
	const auto [priv_key2, pub_key2, address2] = Wallet::get_wallet(path);

	EXPECT_EQ(priv_key1, priv_key2);
	EXPECT_EQ(pub_key1, pub_key2);
	EXPECT_EQ(address1, address2);

	std::remove(path.c_str());
}
