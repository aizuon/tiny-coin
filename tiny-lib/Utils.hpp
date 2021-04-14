#pragma once
#include <cstdint>
#include <vector>
#include <string>

class Utils
{
public:
	static std::string ByteArrayToHexString(const std::vector<uint8_t>& vec);
};