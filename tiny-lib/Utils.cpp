#include "pch.hpp"
#include "Utils.hpp"

#include <chrono>
#include <ranges>
#include <boost/algorithm/hex.hpp>

const bool Utils::IsLittleEndian = IsLittleEndianCast();

std::string Utils::ByteArrayToHexString(const std::vector<uint8_t>& vec)
{
	std::string hash;

	boost::algorithm::hex_lower(vec, std::back_inserter(hash));

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

int64_t Utils::GetUnixTimestamp()
{
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).
		count();
}

bool Utils::IsLittleEndianCast()
{
	const uint32_t i = 1;

	return reinterpret_cast<const uint8_t*>(&i)[0] == i;
}
