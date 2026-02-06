#pragma once
#include <string>

#include "util/uint256_t.hpp"

class HashChecker
{
public:
	static bool is_valid(const std::string& hash, const uint256_t& target_hash);
};
