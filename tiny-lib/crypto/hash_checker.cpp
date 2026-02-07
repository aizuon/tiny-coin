#include "crypto/hash_checker.hpp"

#include <boost/multiprecision/cpp_int.hpp>

bool HashChecker::is_valid(const std::string& hash, const uint256_t& target_hash)
{
	return uint256_t("0x" + hash) < target_hash;
}

bool HashChecker::is_valid(const std::vector<uint8_t>& hash_bytes, const uint256_t& target_hash)
{
	uint256_t hash_value;
	boost::multiprecision::import_bits(hash_value, hash_bytes.begin(), hash_bytes.end(), 8, true);
	return hash_value < target_hash;
}
