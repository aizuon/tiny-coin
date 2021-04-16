#include "pch.hpp"

#include <cstdint>
#include <vector>
#include <string>

#include "../tiny-lib/Utils.hpp"

TEST(UtilsTest, ByteArrayToHexString)
{
	auto hex_string = Utils::ByteArrayToHexString(std::vector<uint8_t>{ 0x00, 0x01, 0x02, 0x03, 0x04 });

	EXPECT_EQ(hex_string, "0001020304");
}

TEST(UtilsTest, HexStringToByteArray)
{
	auto byte_array = Utils::HexStringToByteArray("0001020304");

	std::vector<uint8_t> expected_byte_array{ 0x00, 0x01, 0x02, 0x03, 0x04 };

	EXPECT_EQ(byte_array, expected_byte_array);
}

TEST(UtilsTest, StringToByteArray)
{
	auto byte_array = Utils::StringToByteArray("this is a test string");

	auto expected_byte_array = Utils::HexStringToByteArray("746869732069732061207465737420737472696e67");

	EXPECT_EQ(byte_array, expected_byte_array);
}