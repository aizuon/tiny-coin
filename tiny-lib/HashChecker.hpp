#pragma once
#include <cstdint>
#include <string>
#include <openssl/bn.h>

class HashChecker
{
public:
	static BIGNUM* TargetBitsToBN(uint8_t targetBits);

	static bool IsValid(const std::string& hash, const BIGNUM* target_bn);

private:
};
