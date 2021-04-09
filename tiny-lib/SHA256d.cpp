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

std::string SHA256d::BinaryHashToString(const std::vector<uint8_t>& buffer)
{
    if (buffer.size() != SHA256_DIGEST_LENGTH)
        throw std::exception("SHA256d::BinaryHashToString --- buffer.size() != SHA256_DIGEST_LENGTH");

    std::string hash;
    hash.resize(SHA256_DIGEST_LENGTH * 2);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf_s(hash.data() + (i * 2), 2, "%02x", buffer[i]);

    return hash;
}
