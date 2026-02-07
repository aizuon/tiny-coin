#include "util/utils.hpp"

#include <chrono>
#include <boost/algorithm/hex.hpp>

std::string Utils::byte_array_to_hex_string(const std::vector<uint8_t>& vec)
{
	std::string hash;
	hash.reserve(vec.size() * 2);

	boost::algorithm::hex_lower(vec, std::back_inserter(hash));

	return hash;
}

std::vector<uint8_t> Utils::hex_string_to_byte_array(const std::string& str)
{
	std::vector<uint8_t> vec;
	vec.reserve(str.size() / 2);

	boost::algorithm::unhex(str, std::back_inserter(vec));

	return vec;
}

std::vector<uint8_t> Utils::string_to_byte_array(const std::string& str)
{
	std::vector<uint8_t> vec(str.begin(), str.end());

	return vec;
}

int64_t Utils::get_unix_timestamp()
{
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).
		count();
}
