#include <cstdint>
#include <string>
#include <vector>

#include "util/binary_buffer.hpp"
#include <gtest/gtest.h>

TEST(BinaryBufferTest, PrimitiveReadWrite)
{
	BinaryBuffer buffer;

	bool b_real = true;
	buffer.write(b_real);
	bool b_read = false;
	ASSERT_TRUE(buffer.read(b_read));
	EXPECT_EQ(b_real, b_read);

	uint8_t u8 = 3;
	buffer.write(u8);
	uint8_t u8_read = 0;
	ASSERT_TRUE(buffer.read(u8_read));
	EXPECT_EQ(u8, u8_read);

	int8_t i8 = -5;
	buffer.write(i8);
	int8_t i8_read = 0;
	ASSERT_TRUE(buffer.read(i8_read));
	EXPECT_EQ(i8, i8_read);

	uint16_t u16 = 10000;
	buffer.write(u16);
	uint16_t u16_read = 0;
	ASSERT_TRUE(buffer.read(u16_read));
	EXPECT_EQ(u16, u16_read);

	int16_t i16 = -5000;
	buffer.write(i16);
	int16_t i16_read = 0;
	ASSERT_TRUE(buffer.read(i16_read));
	EXPECT_EQ(i16, i16_read);

	uint32_t ui32 = 7000000;
	buffer.write(ui32);
	uint32_t ui32_read = 0;
	ASSERT_TRUE(buffer.read(ui32_read));
	EXPECT_EQ(ui32, ui32_read);

	int32_t i32 = -3000000;
	buffer.write(i32);
	int32_t i32_read = 0;
	ASSERT_TRUE(buffer.read(i32_read));
	EXPECT_EQ(i32, i32_read);

	uint64_t ui64 = 4000000000;
	buffer.write(ui64);
	uint64_t ui64_read = 0;
	ASSERT_TRUE(buffer.read(ui64_read));
	EXPECT_EQ(ui64, ui64_read);

	int64_t i64 = -2000000000;
	buffer.write(i64);
	int64_t i64_read = 0;
	ASSERT_TRUE(buffer.read(i64_read));
	EXPECT_EQ(i64, i64_read);
}

TEST(BinaryBufferTest, StringReadWrite)
{
	BinaryBuffer buffer;

	const std::string str = "foo";
	buffer.write(str);
	std::string str_read;
	ASSERT_TRUE(buffer.read(str_read));
	EXPECT_EQ(str, str_read);
}

TEST(BinaryBufferTest, VectorReadWrite)
{
	BinaryBuffer buffer;

	std::vector<uint8_t> u8 = { 3, 5, 7, 9, 11, 55, 75 };
	buffer.write(u8);
	std::vector<uint8_t> u8_read;
	ASSERT_TRUE(buffer.read(u8_read));
	EXPECT_EQ(u8, u8_read);

	std::vector<int8_t> i8 = { -6, -14, -32, -44, -65, -77, -99, -102 };
	buffer.write(i8);
	std::vector<int8_t> i8_read;
	ASSERT_TRUE(buffer.read(i8_read));
	EXPECT_EQ(i8, i8_read);

	std::vector<uint16_t> u16 = { 10000, 20000, 30000, 40000, 50000 };
	buffer.write(u16);
	std::vector<uint16_t> u16_read;
	ASSERT_TRUE(buffer.read(u16_read));
	EXPECT_EQ(u16, u16_read);

	std::vector<int16_t> i16 = { -5000, -6000, -7000, -8000, -9000, -10000 };
	buffer.write(i16);
	std::vector<int16_t> i16_read;
	ASSERT_TRUE(buffer.read(i16_read));
	EXPECT_EQ(i16, i16_read);

	std::vector<uint32_t> ui32 = { 7000000, 8000000, 9000000 };
	buffer.write(ui32);
	std::vector<uint32_t> ui32_read;
	ASSERT_TRUE(buffer.read(ui32_read));
	EXPECT_EQ(ui32, ui32_read);

	std::vector i32 = { -3000000, -4000000, -5000000 };
	buffer.write(i32);
	std::vector<int32_t> i32_read;
	ASSERT_TRUE(buffer.read(i32_read));
	EXPECT_EQ(i32, i32_read);

	std::vector<uint64_t> ui64 = { 4000000000, 5000000000, 6000000000 };
	buffer.write(ui64);
	std::vector<uint64_t> ui64_read;
	ASSERT_TRUE(buffer.read(ui64_read));
	EXPECT_EQ(ui64, ui64_read);

	std::vector<int64_t> i64 = { -2000000000, -5000000000, -8000000000 };
	buffer.write(i64);
	std::vector<int64_t> i64_read;
	ASSERT_TRUE(buffer.read(i64_read));
	EXPECT_EQ(i64, i64_read);
}

TEST(BinaryBufferTest, VectorConstructor)
{
	std::vector<uint8_t> vec{ 2, 3, 4, 5 };
	BinaryBuffer buffer(vec);

	const uint8_t new_value = 6;

	vec.push_back(new_value);
	EXPECT_NE(vec, buffer.get_buffer());
	EXPECT_EQ(vec.size(), buffer.get_write_offset() + 1);

	buffer.write(new_value);
	EXPECT_EQ(vec, buffer.get_buffer());
}

TEST(BinaryBufferTest, GrowthPolicy)
{
	BinaryBuffer buffer;

	const std::string str = "foo";

	buffer.write(str);
	buffer.write(str);

	auto& buffer_vec = buffer.get_buffer();
	EXPECT_NE(buffer_vec.size(), buffer_vec.max_size());
}

TEST(BinaryBufferTest, ReadFromEmptyBufferFails)
{
	BinaryBuffer buf;

	uint32_t val = 0;
	EXPECT_FALSE(buf.read(val));

	std::string str;
	EXPECT_FALSE(buf.read(str));

	std::vector<uint8_t> vec;
	EXPECT_FALSE(buf.read(vec));
}

TEST(BinaryBufferTest, ReadBeyondWrittenDataFails)
{
	BinaryBuffer buf;
	buf.write(static_cast<uint8_t>(42));

	uint8_t val = 0;
	EXPECT_TRUE(buf.read(val));
	EXPECT_EQ(42, val);

	uint8_t val2 = 0;
	EXPECT_FALSE(buf.read(val2));
}

TEST(BinaryBufferTest, ReadLargerTypeThanAvailable)
{
	BinaryBuffer buf;
	buf.write(static_cast<uint8_t>(1));

	uint32_t val = 0;
	EXPECT_FALSE(buf.read(val));
}

TEST(BinaryBufferTest, EqualityOperator)
{
	BinaryBuffer a, b;
	a.write(static_cast<uint32_t>(42));
	b.write(static_cast<uint32_t>(42));

	EXPECT_EQ(a, b);

	uint32_t val = 0;
	a.read(val);
	EXPECT_NE(a, b);
}

TEST(BinaryBufferTest, WriteRawAndReadRaw)
{
	BinaryBuffer writer;
	std::vector<uint8_t> data{ 0xDE, 0xAD, 0xBE, 0xEF };
	writer.write_raw(data);

	EXPECT_EQ(4, writer.get_size());

	BinaryBuffer reader(writer.get_buffer());

	uint8_t b1 = 0, b2 = 0, b3 = 0, b4 = 0;
	EXPECT_TRUE(reader.read(b1));
	EXPECT_TRUE(reader.read(b2));
	EXPECT_TRUE(reader.read(b3));
	EXPECT_TRUE(reader.read(b4));
	EXPECT_EQ(0xDE, b1);
	EXPECT_EQ(0xAD, b2);
	EXPECT_EQ(0xBE, b3);
	EXPECT_EQ(0xEF, b4);
}

TEST(BinaryBufferTest, StringOverflowSizeFieldReturnsFailure)
{
	BinaryBuffer buf;
	buf.write(static_cast<uint32_t>(UINT32_MAX));

	std::string out;
	EXPECT_FALSE(buf.read(out));
}
