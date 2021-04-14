#include "pch.hpp"

#include <exception>
#include <openssl/sha.h>

#include "SHA256d.hpp"

std::vector<uint8_t> SHA256d::HashBinary(const std::vector<uint8_t>& buffer)
{
    std::vector<uint8_t> hash1(SHA256_DIGEST_LENGTH);

    SHA256_CTX sha256;

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, buffer.data(), buffer.size());
    SHA256_Final(hash1.data(), &sha256);

    std::vector<uint8_t> hash2(SHA256_DIGEST_LENGTH);

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, hash1.data(), hash1.size());
    SHA256_Final(hash2.data(), &sha256);

    return hash2;
}
