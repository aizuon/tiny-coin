#include <cstdint>
#include <string>
#include <vector>

#include "util/utils.hpp"
#include <gtest/gtest.h>

TEST(UtilsTest, byte_array_to_hex_string)
{
	const auto hex_string = Utils::byte_array_to_hex_string(std::vector<uint8_t>{ 0x00, 0x01, 0x02, 0x03, 0x04 });

	EXPECT_EQ("0001020304", hex_string);
}

TEST(UtilsTest, hex_string_to_byte_array)
{
	const auto byte_array = Utils::hex_string_to_byte_array("0001020304");

	const std::vector<uint8_t> asserted_byte_array{ 0x00, 0x01, 0x02, 0x03, 0x04 };

	EXPECT_EQ(byte_array, asserted_byte_array);
}

TEST(UtilsTest, string_to_byte_array)
{
	const auto byte_array = Utils::string_to_byte_array("foo");

	const auto asserted_byte_array = Utils::hex_string_to_byte_array("666F6F");

	EXPECT_EQ(byte_array, asserted_byte_array);
}
