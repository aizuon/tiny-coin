#pragma once
#include <string>

#include "uint256_t.hpp"

class HashChecker
{
public:
	static bool IsValid(const std::string& hash, const uint256_t& target_hash);
};
