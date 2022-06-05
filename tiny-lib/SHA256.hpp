#pragma once
#include <cstdint>
#include <vector>

#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")

#include <openssl/evp.h>

class SHA256
{
	friend class Crypto;

public:
	static std::vector<uint8_t> HashBinary(const std::vector<uint8_t>& buffer);
	static std::vector<uint8_t> DoubleHashBinary(const std::vector<uint8_t>& buffer);

private:
	static EVP_MD* md;
};
