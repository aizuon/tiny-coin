#pragma once
#include <cstdint>
#include <vector>

class RIPEMD160
{
public:
	static std::vector<uint8_t> HashBinary(const std::vector<uint8_t>& buffer);
};
