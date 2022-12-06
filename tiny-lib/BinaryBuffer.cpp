#include "pch.hpp"
#include "BinaryBuffer.hpp"

BinaryBuffer::BinaryBuffer(const std::vector<uint8_t>& obj)
	: Buffer(obj), WriteOffset(obj.size())
{
}

BinaryBuffer::BinaryBuffer(std::vector<uint8_t>&& obj)
	: Buffer(std::move(obj)), WriteOffset(obj.size())
{
}

BinaryBuffer::BinaryBuffer(const BinaryBuffer& obj)
	: Buffer(obj.Buffer), WriteOffset(obj.WriteOffset), ReadOffset(obj.ReadOffset)
{
}

BinaryBuffer::BinaryBuffer(BinaryBuffer&& obj) noexcept
	: Buffer(std::move(obj.Buffer)), WriteOffset(obj.WriteOffset), ReadOffset(obj.ReadOffset)
{
}

BinaryBuffer& BinaryBuffer::operator=(const BinaryBuffer& obj)
{
	Buffer = obj.Buffer;
	WriteOffset = obj.WriteOffset;
	ReadOffset = obj.ReadOffset;

	return *this;
}

BinaryBuffer& BinaryBuffer::operator=(BinaryBuffer&& obj) noexcept
{
	std::swap(Buffer, obj.Buffer);
	std::swap(WriteOffset, obj.WriteOffset);
	std::swap(ReadOffset, obj.ReadOffset);

	return *this;
}

void BinaryBuffer::WriteSize(uint32_t obj)
{
	Write(obj);
}

void BinaryBuffer::Write(const std::string& obj)
{
	std::scoped_lock lock(Mutex);

	const uint32_t size = obj.size();
	WriteSize(size);

	const uint32_t length = size * sizeof(std::string::value_type);
	GrowIfNeeded(length);
	for (auto o : obj)
	{
		Write(o);
	}
}

void BinaryBuffer::WriteRaw(const std::string& obj)
{
	std::scoped_lock lock(Mutex);

	const uint32_t length = obj.size();
	GrowIfNeeded(length);
	for (auto o : obj)
	{
		Write(o);
	}
}

bool BinaryBuffer::ReadSize(uint32_t& obj)
{
	return Read(obj);
}

bool BinaryBuffer::Read(std::string& obj)
{
	std::scoped_lock lock(Mutex);

	uint32_t size = 0;
	if (!ReadSize(size))
		return false;

	const uint32_t length = size * sizeof(std::string::value_type);

	const uint32_t final_offset = ReadOffset + length;
	if (Buffer.size() < final_offset)
		return false;

	obj.resize(size);
	for (uint32_t i = 0; i < size; i++)
	{
		if (!Read(obj[i]))
			return false;
	}

	return true;
}

bool BinaryBuffer::operator==(const BinaryBuffer& obj) const
{
	if (WriteOffset != obj.WriteOffset)
		return false;

	if (ReadOffset != obj.ReadOffset)
		return false;

	return Buffer == obj.Buffer;
}

void BinaryBuffer::GrowIfNeeded(uint32_t writeLength)
{
	const uint32_t final_length = WriteOffset + writeLength;
	const bool reserve_needed = Buffer.capacity() <= final_length;
	const bool resize_needed = Buffer.size() <= final_length;

	if (reserve_needed)
		Buffer.reserve(final_length * BUFFER_GROW_FACTOR);

	if (resize_needed)
		Buffer.resize(final_length);
}
