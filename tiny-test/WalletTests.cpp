#include "pch.hpp"

#include "../tiny-lib/Wallet.hpp"
#include "../tiny-lib/Utils.hpp"

#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")

TEST(WalletTest, PubKeyToAddress)
{
	std::vector<uint8_t> pubKey = Utils::HexStringToByteArray("0250863ad64a87ae8a2fe83c1af1a8403cb53f53e486d8511dad8a04887e5b2352"); 

	EXPECT_EQ(Wallet::PubKeyToAddress(pubKey), "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs");
}