#include "pch.hpp"

#include "BinaryBuffer.hpp"

BinaryBuffer::BinaryBuffer()
	: buffer(std::make_shared<std::vector<uint8_t>>())
{
}

BinaryBuffer::BinaryBuffer(size_t length)
	: buffer(std::make_shared<std::vector<uint8_t>>(length))
{
}

void BinaryBuffer::Write(const std::string& obj)
{
	std::lock_guard<std::mutex> lock(mtx);

	size_t size = obj.size();
	size_t length1 = sizeof(size);
	size_t length2 = size * sizeof(std::string::value_type);

	size_t finalLength = writeOffset + length1 + length2;
	if (buffer->size() <= finalLength)
		buffer->resize(finalLength);

	memcpy(buffer->data() + writeOffset, &size, length1);
	memcpy(buffer->data() + writeOffset + length1, obj.data(), length2);
	writeOffset = finalLength;
}

bool BinaryBuffer::Read(std::string& obj)
{
	std::lock_guard<std::mutex> lock(mtx);

	size_t size = 0;
	size_t length1 = sizeof(size);
	if (buffer->size() < readOffset + length1)
		return false;

	memcpy(&size, buffer->data() + readOffset, length1);

	size_t length2 = size * sizeof(std::string::value_type);

	size_t finalOffset = readOffset + length1 + length2;
	if (buffer->size() < finalOffset)
		return false;

	obj.resize(size);
	memcpy(obj.data(), buffer->data() + readOffset + length1, length2);
	readOffset = finalOffset;

	return true;
}
