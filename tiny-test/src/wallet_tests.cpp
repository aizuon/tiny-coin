#include <cstdint>
#include <vector>

#include "util/utils.hpp"
#include "wallet/wallet.hpp"
#include <gtest/gtest.h>

TEST(WalletTest, pub_key_to_address)
{
	const std::vector<uint8_t> pub_key = Utils::hex_string_to_byte_array(
		"0250863ad64a87ae8a2fe83c1af1a8403cb53f53e486d8511dad8a04887e5b2352");

	const std::string address = Wallet::pub_key_to_address(pub_key);

	EXPECT_EQ("1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs", address);
}
