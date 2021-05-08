#include "pch.hpp"
#include "SHA256.hpp"

#include <openssl/sha.h>

std::vector<uint8_t> SHA256::HashBinary(const std::vector<uint8_t>& buffer)
{
	std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);

	SHA256_CTX sha256;

	if (!SHA256_Init(&sha256) || !SHA256_Update(&sha256, buffer.data(), buffer.size()) || !SHA256_Final(
		hash.data(), &sha256))
		return {};

	return hash;
}

std::vector<uint8_t> SHA256::DoubleHashBinary(const std::vector<uint8_t>& buffer)
{
	return HashBinary(HashBinary(buffer));
}
