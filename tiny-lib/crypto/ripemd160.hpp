#pragma once
#include <cstdint>
#include <vector>

#include <openssl/evp.h>

class RIPEMD160
{
	friend class Crypto;

public:
	static std::vector<uint8_t> hash_binary(const std::vector<uint8_t>& buffer);

private:
	static EVP_MD* md;
};
