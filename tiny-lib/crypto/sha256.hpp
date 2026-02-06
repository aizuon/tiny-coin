#pragma once
#include <cstdint>
#include <vector>

#include <openssl/evp.h>

class SHA256
{
	friend class Crypto;

public:
	static std::vector<uint8_t> hash_binary(const std::vector<uint8_t>& buffer);
	static std::vector<uint8_t> double_hash_binary(const std::vector<uint8_t>& buffer);

private:
	static EVP_MD* md;
};
