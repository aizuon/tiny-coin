#include "pch.hpp"

#include <cstdint>
#include <vector>

#include "../tiny-lib/Utils.hpp"
#include "../tiny-lib/Wallet.hpp"
#include "gtest/gtest.h"

TEST(WalletTest, PubKeyToAddress)
{
	const std::vector<uint8_t> pub_key = Utils::HexStringToByteArray(
		"0250863ad64a87ae8a2fe83c1af1a8403cb53f53e486d8511dad8a04887e5b2352");

	const std::string address = Wallet::PubKeyToAddress(pub_key);

	EXPECT_EQ("1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs", address);
}
