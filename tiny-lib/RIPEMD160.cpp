#include "pch.hpp"
#include "RIPEMD160.hpp"

#include <openssl/ripemd.h>

std::vector<uint8_t> RIPEMD160::HashBinary(const std::vector<uint8_t>& buffer)
{
	std::vector<uint8_t> hash(RIPEMD160_DIGEST_LENGTH);

	RIPEMD160_CTX ripemd160;

	if (!RIPEMD160_Init(&ripemd160) || !RIPEMD160_Update(&ripemd160, buffer.data(), buffer.size()) || !RIPEMD160_Final(
		hash.data(), &ripemd160))
		return std::vector<uint8_t>();

	return hash;
}
