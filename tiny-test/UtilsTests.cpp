#include "pch.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include "../tiny-lib/Utils.hpp"

TEST(UtilsTest, ByteArrayToHexString)
{
	const auto hex_string = Utils::ByteArrayToHexString(std::vector<uint8_t>{ 0x00, 0x01, 0x02, 0x03, 0x04 });

	EXPECT_EQ(hex_string, "0001020304");
}

TEST(UtilsTest, HexStringToByteArray)
{
	const auto byte_array = Utils::HexStringToByteArray("0001020304");

	const std::vector<uint8_t> asserted_byte_array{ 0x00, 0x01, 0x02, 0x03, 0x04 };

	EXPECT_EQ(byte_array, asserted_byte_array);
}

TEST(UtilsTest, StringToByteArray)
{
	const auto byte_array = Utils::StringToByteArray("foo");

	const auto asserted_byte_array = Utils::HexStringToByteArray("666F6F");

	EXPECT_EQ(byte_array, asserted_byte_array);
}
