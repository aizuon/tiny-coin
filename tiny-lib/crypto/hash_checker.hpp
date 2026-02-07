#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "util/uint256_t.hpp"

class HashChecker
{
public:
	static bool is_valid(const std::string& hash, const uint256_t& target_hash);
	static bool is_valid(const std::vector<uint8_t>& hash_bytes, const uint256_t& target_hash);
};
