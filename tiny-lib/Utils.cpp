#include "pch.hpp"
#include "Utils.hpp"

#include <chrono>
#include <iomanip>
#include <ranges>
#include <sstream>
#include <boost/algorithm/hex.hpp>

const bool Utils::IsBigEndian = IsBigEndianCast();

std::string Utils::ByteArrayToHexString(const std::vector<uint8_t>& vec)
{
	std::string hash;

	boost::algorithm::hex_lower(vec.begin(), vec.end(), back_inserter(hash));

	return hash;
}

std::vector<uint8_t> Utils::HexStringToByteArray(const std::string& str)
{
	std::string hash = boost::algorithm::unhex(str);

	std::vector<uint8_t> vec(hash.size());

	std::ranges::copy(hash, vec.data());

	return vec;
}

std::vector<uint8_t> Utils::StringToByteArray(const std::string& str)
{
	std::vector<uint8_t> vec(str.begin(), str.end());

	return vec;
}

std::string Utils::ByteArrayToHexString_DEBUG(const std::vector<uint8_t>& vec)
{
	std::stringstream ss;
	ss << std::hex;

	for (auto b : vec)
		ss << std::setw(2) << std::setfill('0') << static_cast<int>(b) << " ";

	std::string output = ss.str();
	std::ranges::transform(output, output.begin(), std::toupper);

	return output;
}

int64_t Utils::GetUnixTimestamp()
{
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).
		count();
}

bool Utils::IsBigEndianCast()
{
	const uint32_t i = 1;

	return (reinterpret_cast<const uint8_t*>(&i)[sizeof(i) - 1] == i);
}
