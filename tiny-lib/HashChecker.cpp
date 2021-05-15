#include "pch.hpp"
#include "HashChecker.hpp"

bool HashChecker::IsValid(const std::string& hash, const uint256_t& target_hash)
{
	return uint256_t("0x" + hash) < target_hash;
}
