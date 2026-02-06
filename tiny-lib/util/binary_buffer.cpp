#include "util/binary_buffer.hpp"

BinaryBuffer::BinaryBuffer(const std::vector<uint8_t>& obj)
	: buffer_(obj), write_offset_(static_cast<uint32_t>(obj.size()))
{}

BinaryBuffer::BinaryBuffer(std::vector<uint8_t>&& obj)
	: buffer_(std::move(obj)), write_offset_(static_cast<uint32_t>(buffer_.size()))
{}

BinaryBuffer::BinaryBuffer(const BinaryBuffer& obj)
	: buffer_(obj.buffer_), write_offset_(obj.write_offset_), read_offset_(obj.read_offset_)
{}

BinaryBuffer::BinaryBuffer(BinaryBuffer&& obj) noexcept
	: buffer_(std::move(obj.buffer_)), write_offset_(obj.write_offset_), read_offset_(obj.read_offset_)
{}

BinaryBuffer& BinaryBuffer::operator=(const BinaryBuffer& obj)
{
	buffer_ = obj.buffer_;
	write_offset_ = obj.write_offset_;
	read_offset_ = obj.read_offset_;

	return *this;
}

BinaryBuffer& BinaryBuffer::operator=(BinaryBuffer&& obj) noexcept
{
	std::swap(buffer_, obj.buffer_);
	std::swap(write_offset_, obj.write_offset_);
	std::swap(read_offset_, obj.read_offset_);

	return *this;
}

void BinaryBuffer::write_size(uint32_t obj)
{
	write(obj);
}

void BinaryBuffer::write(const std::string& obj)
{
	std::scoped_lock lock(mutex_);

	const uint32_t size = static_cast<uint32_t>(obj.size());
	write_size(size);

	const uint32_t length = size * sizeof(std::string::value_type);
	grow_if_needed(length);
	for (auto o : obj)
	{
		write(o);
	}
}

void BinaryBuffer::write_raw(const std::string& obj)
{
	std::scoped_lock lock(mutex_);

	const uint32_t length = static_cast<uint32_t>(obj.size());
	grow_if_needed(length);
	for (auto o : obj)
	{
		write(o);
	}
}

bool BinaryBuffer::read_size(uint32_t& obj)
{
	return read(obj);
}

bool BinaryBuffer::read(std::string& obj)
{
	std::scoped_lock lock(mutex_);

	uint32_t size = 0;
	if (!read_size(size))
		return false;

	const uint32_t length = size * sizeof(std::string::value_type);

	const uint32_t final_offset = read_offset_ + length;
	if (buffer_.size() < final_offset)
		return false;

	obj.resize(size);
	for (uint32_t i = 0; i < size; i++)
	{
		if (!read(obj[i]))
			return false;
	}

	return true;
}

bool BinaryBuffer::operator==(const BinaryBuffer& obj) const
{
	if (write_offset_ != obj.write_offset_)
		return false;

	if (read_offset_ != obj.read_offset_)
		return false;

	return buffer_ == obj.buffer_;
}

void BinaryBuffer::grow_if_needed(uint32_t write_length)
{
	const uint32_t final_length = write_offset_ + write_length;
	const bool reserve_needed = buffer_.capacity() < final_length;
	const bool resize_needed = buffer_.size() < final_length;

	if (reserve_needed)
		buffer_.reserve(final_length + final_length / 2);

	if (resize_needed)
		buffer_.resize(final_length);
}
