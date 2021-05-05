#pragma once
#include <cstdint>
#include <string>
#include <openssl/bn.h>

#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")

class HashChecker
{
public:
	static BIGNUM* TargetBitsToBN(uint8_t targetBits);

	static bool IsValid(const std::string& hash, const BIGNUM* target_bn);
};
