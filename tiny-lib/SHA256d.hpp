#pragma once
#include <cstdint>
#include <vector>

class SHA256d
{
public:
	static std::vector<uint8_t> HashBinary(const std::vector<uint8_t>& buffer);
};