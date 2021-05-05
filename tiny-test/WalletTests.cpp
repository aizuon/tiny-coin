#include "pch.hpp"

#include <cstdint>
#include <vector>

#include "../tiny-lib/Utils.hpp"
#include "../tiny-lib/Wallet.hpp"
#include "gtest/gtest.h"

#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")

TEST(WalletTest, PubKeyToAddress)
{
	const std::vector<uint8_t> pubKey = Utils::HexStringToByteArray(
		"0250863ad64a87ae8a2fe83c1af1a8403cb53f53e486d8511dad8a04887e5b2352");

	std::string address = Wallet::PubKeyToAddress(pubKey);

	EXPECT_EQ(address, "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs");
}
