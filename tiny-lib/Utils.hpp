#pragma once
#include <cstdint>
#include <string>
#include <vector>

class Utils
{
public:
	static std::string ByteArrayToHexString(const std::vector<uint8_t>& vec);
	static std::vector<uint8_t> HexStringToByteArray(const std::string& str);
	static std::vector<uint8_t> StringToByteArray(const std::string& str);

	static std::string ByteArrayToHexString_DEBUG(const std::vector<uint8_t>& vec);

	static int64_t GetUnixTimestamp();

	static const bool IsBigEndian;

private:
	static bool IsBigEndianCast();
};
