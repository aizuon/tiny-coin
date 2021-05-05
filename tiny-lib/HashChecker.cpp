#include "pch.hpp"

#include "HashChecker.hpp"

BIGNUM* HashChecker::TargetBitsToBN(uint8_t targetBits)
{
	BIGNUM* one_bn = BN_new();
	if (!BN_hex2bn(&one_bn, "1"))
	{
		BN_free(one_bn);

		return nullptr;
	}

	BIGNUM* target_bn = BN_new();
	const uint8_t target_lshift = 256 - targetBits;
	if (!BN_lshift(target_bn, one_bn, target_lshift))
	{
		BN_free(one_bn);
		BN_free(target_bn);

		return nullptr;
	}

	BN_free(one_bn);

	return target_bn;
}

bool HashChecker::IsValid(const std::string& hash, const BIGNUM* target_bn)
{
	BIGNUM* hash_bn = BN_new();
	if (!BN_hex2bn(&hash_bn, hash.c_str()))
	{
		BN_free(hash_bn);

		return false;
	}

	const int res = BN_cmp(hash_bn, target_bn);

	BN_free(hash_bn);

	return res == -1;
}
