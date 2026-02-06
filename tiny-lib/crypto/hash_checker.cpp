#include "crypto/hash_checker.hpp"

bool HashChecker::is_valid(const std::string& hash, const uint256_t& target_hash)
{
	return uint256_t("0x" + hash) < target_hash;
}
