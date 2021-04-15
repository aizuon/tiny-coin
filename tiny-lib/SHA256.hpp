#pragma once
#include <cstdint>
#include <vector>

class SHA256
{
public:
	static std::vector<uint8_t> HashBinary(const std::vector<uint8_t>& buffer);
	static std::vector<uint8_t> DoubleHashBinary(const std::vector<uint8_t>& buffer);
};