#include "pch.hpp"

#include "Wallet.hpp"
#include "SHA256.hpp"
#include "RIPEMD160.hpp"
#include "Base58.hpp"

std::string Wallet::PubKeyToAddress(const std::vector<uint8_t>& pubKey)
{
    auto sha256 = SHA256::HashBinary(pubKey);

    auto ripe = RIPEMD160::HashBinary(sha256);

    ripe.insert(ripe.begin(), 0x00);

    auto sha256d = SHA256::DoubleHashBinary(ripe);

    std::vector<uint8_t> checksum(sha256d.begin(), sha256d.begin() + 4);

    ripe.insert(ripe.end(), checksum.begin(), checksum.end());

    return PubKeyHashVersion + Base58::Encode(ripe);
}
