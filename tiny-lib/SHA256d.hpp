#pragma once
#include <cstdint>
#include <vector>
#include <string>

class SHA256d
{
public:
	static std::vector<uint8_t> HashBinary(const std::vector<uint8_t>& buffer);

	static std::string BinaryHashToString(const std::vector<uint8_t>& buffer);
};