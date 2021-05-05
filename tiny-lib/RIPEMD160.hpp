#pragma once
#include <cstdint>
#include <vector>

#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")

class RIPEMD160
{
public:
	static std::vector<uint8_t> HashBinary(const std::vector<uint8_t>& buffer);
};
