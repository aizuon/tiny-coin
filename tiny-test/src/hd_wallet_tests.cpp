#include <cstdint>
#include <string>
#include <vector>

#include "wallet/hd_key_chain.hpp"
#include "wallet/hd_wallet.hpp"
#include "wallet/wallet.hpp"
#include "util/utils.hpp"
#include <gtest/gtest.h>

TEST(HDWalletTest, BIP32_TestVectors)
{
    {
        const auto seed = Utils::hex_string_to_byte_array(
            "000102030405060708090a0b0c0d0e0f");
        const auto master = HDKeyChain::from_seed(seed);

        EXPECT_EQ("e8f32e723decf4051aefac8e2c93c9c5b214313817cdb01a1494b917c8436b35",
            Utils::byte_array_to_hex_string(master.key));
        EXPECT_EQ("873dff81c02f525623fd1fe5167eac3a55a049de3d314bb42ee227ffed37d508",
            Utils::byte_array_to_hex_string(master.chain_code));
        EXPECT_EQ(0, master.depth);

        const auto deep = HDKeyChain::derive_path(master, "m/0'/1/2'/2/1000000000");
        EXPECT_EQ("471b76e389e528d6de6d816857e012c5455051cad6660850e58372a6c3e6e7c8",
            Utils::byte_array_to_hex_string(deep.key));
        EXPECT_EQ(5, deep.depth);
    }

    {
        const auto seed = Utils::hex_string_to_byte_array(
            "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a2"
            "9f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542");
        const auto master = HDKeyChain::from_seed(seed);

        EXPECT_EQ("4b03d6fc340455b363f51020ad3ecca4f0850280cf436c70c727923f6db46c3e",
            Utils::byte_array_to_hex_string(master.key));
        EXPECT_EQ("60499f801b896d83179a4374aeb7822aaeaceaa0db1f85ee3e904c4defbd9689",
            Utils::byte_array_to_hex_string(master.chain_code));
    }
}

TEST(HDWalletTest, HDWallet_SeedRoundTrip)
{
    auto wallet = HDWallet::create();

    EXPECT_EQ(HDWallet::SEED_SIZE, wallet.get_seed().size());

    auto wallet2 = HDWallet::from_seed(wallet.get_seed());
    EXPECT_EQ(wallet.get_primary_address(), wallet2.get_primary_address());
}

TEST(HDWalletTest, HDWallet_AddressDerivationAndKeys)
{
    auto wallet = HDWallet::create();

    const auto addr1 = wallet.get_new_address();
    const auto addr2 = wallet.get_new_address();
    EXPECT_NE(addr1, addr2);

    const auto change1 = wallet.get_change_address();
    const auto change2 = wallet.get_change_address();
    EXPECT_NE(change1, change2);
    EXPECT_NE(addr1, change1);

    EXPECT_TRUE(wallet.owns_address(addr1));
    EXPECT_FALSE(wallet.owns_address("1InvalidAddress"));

    std::vector<uint8_t> priv_key, pub_key;
    EXPECT_TRUE(wallet.get_keys_for_address(addr1, priv_key, pub_key));
    EXPECT_EQ(addr1, Wallet::pub_key_to_address(pub_key));

    const uint32_t ext_before = wallet.get_external_index();
    wallet.get_change_address();
    EXPECT_EQ(ext_before, wallet.get_external_index());
}

TEST(HDWalletTest, HDWallet_SaveLoadRoundTrip)
{
    const std::string path = "test_hd_wallet.dat";

    auto wallet = HDWallet::create();
    const auto addr1 = wallet.get_new_address();
    const auto addr2 = wallet.get_new_address();
    const auto change1 = wallet.get_change_address();
    wallet.save(path);

    auto loaded = HDWallet::load(path);
    EXPECT_EQ(wallet.get_seed(), loaded.get_seed());
    EXPECT_EQ(wallet.get_external_index(), loaded.get_external_index());
    EXPECT_EQ(wallet.get_internal_index(), loaded.get_internal_index());
    EXPECT_EQ(wallet.get_primary_address(), loaded.get_primary_address());
    EXPECT_TRUE(loaded.owns_address(addr1));
    EXPECT_TRUE(loaded.owns_address(addr2));
    EXPECT_TRUE(loaded.owns_address(change1));

    std::remove(path.c_str());
}
