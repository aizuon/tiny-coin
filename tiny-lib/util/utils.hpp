#pragma once
#include <bit>
#include <cstdint>
#include <string>
#include <vector>

class Utils
{
public:
	static std::string byte_array_to_hex_string(const std::vector<uint8_t>& vec);
	static std::vector<uint8_t> hex_string_to_byte_array(const std::string& str);
	static std::vector<uint8_t> string_to_byte_array(const std::string& str);

	static int64_t get_unix_timestamp();

	static constexpr bool is_little_endian = std::endian::native == std::endian::little;

};
