#include "pch.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include "../tiny-lib/BinaryBuffer.hpp"
#include "gtest/gtest.h"

TEST(BinaryBufferTest, PrimitiveReadWrite)
{
	BinaryBuffer buffer;

	bool b_real = true;
	buffer.Write(b_real);
	bool b_read = false;
	ASSERT_TRUE(buffer.Read(b_read));
	EXPECT_EQ(b_real, b_read);

	uint8_t u8 = 3;
	buffer.Write(u8);
	uint8_t u8_read = 0;
	ASSERT_TRUE(buffer.Read(u8_read));
	EXPECT_EQ(u8, u8_read);

	int8_t i8 = -5;
	buffer.Write(i8);
	int8_t i8_read = 0;
	ASSERT_TRUE(buffer.Read(i8_read));
	EXPECT_EQ(i8, i8_read);

	uint16_t u16 = 10000;
	buffer.Write(u16);
	uint16_t u16_read = 0;
	ASSERT_TRUE(buffer.Read(u16_read));
	EXPECT_EQ(u16, u16_read);

	int16_t i16 = -5000;
	buffer.Write(i16);
	int16_t i16_read = 0;
	ASSERT_TRUE(buffer.Read(i16_read));
	EXPECT_EQ(i16, i16_read);

	uint32_t ui32 = 7000000;
	buffer.Write(ui32);
	uint32_t ui32_read = 0;
	ASSERT_TRUE(buffer.Read(ui32_read));
	EXPECT_EQ(ui32, ui32_read);

	int32_t i32 = -3000000;
	buffer.Write(i32);
	int32_t i32_read = 0;
	ASSERT_TRUE(buffer.Read(i32_read));
	EXPECT_EQ(i32, i32_read);

	uint64_t ui64 = 4000000000;
	buffer.Write(ui64);
	uint64_t ui64_read = 0;
	ASSERT_TRUE(buffer.Read(ui64_read));
	EXPECT_EQ(ui64, ui64_read);

	int64_t i64 = -2000000000;
	buffer.Write(i64);
	int64_t i64_read = 0;
	ASSERT_TRUE(buffer.Read(i64_read));
	EXPECT_EQ(i64, i64_read);
}

TEST(BinaryBufferTest, StringReadWrite)
{
	BinaryBuffer buffer;

	std::string str = "foo";
	buffer.Write(str);
	std::string str_read;
	ASSERT_TRUE(buffer.Read(str_read));
	EXPECT_EQ(str, str_read);
}

TEST(BinaryBufferTest, VectorReadWrite)
{
	BinaryBuffer buffer;

	std::vector<uint8_t> u8 = {3, 5, 7, 9, 11, 55, 75};
	buffer.Write(u8);
	std::vector<uint8_t> u8_read;
	ASSERT_TRUE(buffer.Read(u8_read));
	EXPECT_EQ(u8, u8_read);

	std::vector<int8_t> i8 = {-6, -14, -32, -44, -65, -77, -99, -102};
	buffer.Write(i8);
	std::vector<int8_t> i8_read;
	ASSERT_TRUE(buffer.Read(i8_read));
	EXPECT_EQ(i8, i8_read);

	std::vector<uint16_t> u16 = {10000, 20000, 30000, 40000, 50000};
	buffer.Write(u16);
	std::vector<uint16_t> u16_read;
	ASSERT_TRUE(buffer.Read(u16_read));
	EXPECT_EQ(u16, u16_read);

	std::vector<int16_t> i16 = {-5000, -6000, -7000, -8000, -9000, -10000};
	buffer.Write(i16);
	std::vector<int16_t> i16_read;
	ASSERT_TRUE(buffer.Read(i16_read));
	EXPECT_EQ(i16, i16_read);

	std::vector<uint32_t> ui32 = {7000000, 8000000, 9000000};
	buffer.Write(ui32);
	std::vector<uint32_t> ui32_read;
	ASSERT_TRUE(buffer.Read(ui32_read));
	EXPECT_EQ(ui32, ui32_read);

	std::vector i32 = {-3000000, -4000000, -5000000};
	buffer.Write(i32);
	std::vector<int32_t> i32_read;
	ASSERT_TRUE(buffer.Read(i32_read));
	EXPECT_EQ(i32, i32_read);

	std::vector<uint64_t> ui64 = {4000000000, 5000000000, 6000000000};
	buffer.Write(ui64);
	std::vector<uint64_t> ui64_read;
	ASSERT_TRUE(buffer.Read(ui64_read));
	EXPECT_EQ(ui64, ui64_read);

	std::vector<int64_t> i64 = {-2000000000, -5000000000, -8000000000};
	buffer.Write(i64);
	std::vector<int64_t> i64_read;
	ASSERT_TRUE(buffer.Read(i64_read));
	EXPECT_EQ(i64, i64_read);
}

TEST(BinaryBufferTest, VectorConstructor)
{
	std::vector<uint8_t> vec{2, 3, 4, 5};
	BinaryBuffer buffer(vec);

	const uint8_t new_value = 6;

	vec.push_back(new_value);
	EXPECT_NE(vec, buffer.GetBuffer());
	EXPECT_EQ(vec.size(), buffer.GetWriteOffset() + 1);

	buffer.Write(new_value);
	EXPECT_EQ(vec, buffer.GetBuffer());
}

TEST(BinaryBufferTest, GrowthPolicy)
{
	BinaryBuffer buffer;

	const std::string str = "foo";

	buffer.Write(str);
	buffer.Write(str);

	auto& buffer_vec = buffer.GetBuffer();
	EXPECT_NE(buffer_vec.size(), buffer_vec.max_size());
}
