#include "pch.hpp"

#include "BinaryBuffer.hpp"

BinaryBuffer::BinaryBuffer(const std::vector<uint8_t>& obj)
	: Buffer(obj), WriteOffset(obj.size())
{

}

BinaryBuffer::BinaryBuffer(std::vector<uint8_t>&& obj)
	: Buffer(obj), WriteOffset(obj.size())
{
}

BinaryBuffer::BinaryBuffer(const BinaryBuffer& obj)
{
	Buffer = obj.Buffer;
	WriteOffset = obj.WriteOffset;
	ReadOffset = obj.ReadOffset;
}

BinaryBuffer& BinaryBuffer::operator=(const BinaryBuffer& obj)
{
	Buffer = obj.Buffer;
	WriteOffset = obj.WriteOffset;
	ReadOffset = obj.ReadOffset;

	return *this;
}

void BinaryBuffer::Write(const std::string& obj)
{
	std::lock_guard<std::mutex> lock(Mutex);

	size_t size = obj.size();
	size_t length1 = sizeof(size);
	size_t length2 = size * sizeof(std::string::value_type);

	size_t finalLength = WriteOffset + length1 + length2;
	GrowIfNeeded(finalLength);

	memcpy(Buffer.data() + WriteOffset, &size, length1);
	memcpy(Buffer.data() + WriteOffset + length1, obj.data(), length2);
	WriteOffset = finalLength;
}

bool BinaryBuffer::Read(std::string& obj)
{
	std::lock_guard<std::mutex> lock(Mutex);

	size_t size = 0;
	size_t length1 = sizeof(size);
	if (Buffer.size() < ReadOffset + length1)
		return false;

	memcpy(&size, Buffer.data() + ReadOffset, length1);

	size_t length2 = size * sizeof(std::string::value_type);

	size_t finalOffset = ReadOffset + length1 + length2;
	if (Buffer.size() < finalOffset)
		return false;

	obj.resize(size);
	memcpy(obj.data(), Buffer.data() + ReadOffset + length1, length2);
	ReadOffset = finalOffset;

	return true;
}

void BinaryBuffer::GrowIfNeeded(size_t finalLength)
{
	bool reserve_needed = Buffer.capacity() <= finalLength;
	bool resize_needed = Buffer.size() <= finalLength;

	if (reserve_needed)
		Buffer.reserve(finalLength * BUFFER_GROW_FACTOR);

	if (resize_needed)
		Buffer.resize(finalLength);
}
