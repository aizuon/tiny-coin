#include "util/binary_buffer.hpp"

BinaryBuffer::BinaryBuffer(const std::vector<uint8_t>& obj)
	: buffer_(obj), write_offset_(static_cast<uint32_t>(obj.size()))
{}

BinaryBuffer::BinaryBuffer(std::vector<uint8_t>&& obj)
	: buffer_(std::move(obj)), write_offset_(static_cast<uint32_t>(buffer_.size()))
{}

void BinaryBuffer::write_size(uint32_t obj)
{
	write(obj);
}

void BinaryBuffer::write(const std::string& obj)
{
	const uint32_t size = static_cast<uint32_t>(obj.size());
	write_size(size);

	grow_if_needed(size);
	std::memcpy(buffer_.data() + write_offset_, obj.data(), size);
	write_offset_ += size;
}

void BinaryBuffer::write_raw(const std::string& obj)
{
	const uint32_t length = static_cast<uint32_t>(obj.size());
	grow_if_needed(length);
	std::memcpy(buffer_.data() + write_offset_, obj.data(), length);
	write_offset_ += length;
}

bool BinaryBuffer::read_size(uint32_t& obj)
{
	return read(obj);
}

bool BinaryBuffer::read(std::string& obj)
{
	uint32_t size = 0;
	if (!read_size(size))
		return false;

	if (size > UINT32_MAX - read_offset_)
		return false;

	const uint32_t final_offset = read_offset_ + size;
	if (buffer_.size() < final_offset)
		return false;

	obj.assign(reinterpret_cast<const char*>(buffer_.data() + read_offset_), size);
	read_offset_ = final_offset;

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
	if (write_length > UINT32_MAX - write_offset_)
		return;

	const uint32_t final_length = write_offset_ + write_length;
	const bool reserve_needed = buffer_.capacity() < final_length;
	const bool resize_needed = buffer_.size() < final_length;

	if (reserve_needed)
		buffer_.reserve(final_length + final_length / 2);

	if (resize_needed)
		buffer_.resize(final_length);
}
