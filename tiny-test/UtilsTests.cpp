#include "pch.hpp"

#include <cstdint>
#include <vector>
#include <string>

#include "../tiny-lib/Utils.hpp"

TEST(UtilsTest, ByteArrayToHexString)
{
	auto hex_string = Utils::ByteArrayToHexString(std::vector<uint8_t>{0x00, 0x01, 0x02, 0x03, 0x04});

	EXPECT_EQ(hex_string, "0001020304");
}

TEST(UtilsTest, HexStringToByteArray)
{
	auto byte_array = Utils::HexStringToByteArray("0001020304");

	std::vector<uint8_t> ASSERTed_byte_array{0x00, 0x01, 0x02, 0x03, 0x04};

	EXPECT_EQ(byte_array, ASSERTed_byte_array);
}

TEST(UtilsTest, StringToByteArray)
{
	auto byte_array = Utils::StringToByteArray("foo");

	auto ASSERTed_byte_array = Utils::HexStringToByteArray("666F6F");

	EXPECT_EQ(byte_array, ASSERTed_byte_array);
}
