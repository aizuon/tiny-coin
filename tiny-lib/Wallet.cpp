#include "pch.hpp"

#include <openssl/sha.h>
#include <openssl/ripemd.h>

#include "Wallet.hpp"
#include "SHA256d.hpp"
#include "Base58.hpp"

std::string Wallet::PubKeyToAddress(const std::vector<uint8_t>& pubKey)
{
    std::vector<uint8_t> hash1(SHA256_DIGEST_LENGTH);

    SHA256_CTX sha256;

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, pubKey.data(), pubKey.size());
    SHA256_Final(hash1.data(), &sha256);

    std::vector<uint8_t> hash2(RIPEMD160_DIGEST_LENGTH);
    RIPEMD160_CTX ripemd160;
    RIPEMD160_Init(&ripemd160);
    RIPEMD160_Update(&ripemd160, hash1.data(), hash1.size());
    RIPEMD160_Final(hash2.data(), &ripemd160);

    hash2.insert(hash2.begin(), 0x00);

    auto hash3 = SHA256d::HashBinary(hash2);

    std::vector<uint8_t> checksum(hash3.begin(), hash3.begin() + 4);

    hash2.insert(hash2.end(), checksum.begin(), checksum.end());

    return PubKeyHashVersion + Base58::Encode(hash2);
}
