#pragma once
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <boost/endian/conversion.hpp>

#include "util/utils.hpp"

class BinaryBuffer
{
public:
	BinaryBuffer() = default;
	BinaryBuffer(const std::vector<uint8_t>& obj);
	BinaryBuffer(std::vector<uint8_t>&& obj);

	inline const std::vector<uint8_t>& get_buffer() const
	{
		return buffer_;
	}

	inline std::vector<uint8_t>& get_writable_buffer()
	{
		return buffer_;
	}

	inline uint32_t get_size() const
	{
		return static_cast<uint32_t>(buffer_.size());
	}

	inline uint32_t get_write_offset() const
	{
		return write_offset_;
	}

	inline uint32_t get_read_offset() const
	{
		return read_offset_;
	}

	inline void grow_to(uint32_t size)
	{
		if (size <= buffer_.size())
			return;

		buffer_.resize(size);
	}

	inline void reserve(uint32_t size)
	{
		buffer_.reserve(size);
	}

	void write_size(uint32_t obj);

	template <typename T>
	void write(T obj)
	{
		static_assert(std::is_arithmetic_v<T>);

		if (!Utils::is_little_endian)
		{
			boost::endian::endian_reverse_inplace(obj);
		}

		const uint32_t length = sizeof(T);
		grow_if_needed(length);
		std::memcpy(buffer_.data() + write_offset_, &obj, length);
		write_offset_ += length;
	}

	template <typename T>
	void write(const std::vector<T>& obj)
	{
		static_assert(std::is_arithmetic_v<T>);

		const uint32_t size = static_cast<uint32_t>(obj.size());
		write_size(size);

		if constexpr (sizeof(T) == 1)
		{
			const uint32_t length = size;
			grow_if_needed(length);
			std::memcpy(buffer_.data() + write_offset_, obj.data(), length);
			write_offset_ += length;
		}
		else
		{
			for (auto o : obj)
			{
				write(o);
			}
		}
	}

	template <typename T>
	void write_raw(const std::vector<T>& obj)
	{
		const size_t byte_length = obj.size() * sizeof(T);
		if (byte_length > UINT32_MAX)
			return;
		const uint32_t length = static_cast<uint32_t>(byte_length);
		grow_if_needed(length);
		if constexpr (sizeof(T) == 1)
		{
			std::memcpy(buffer_.data() + write_offset_, obj.data(), length);
			write_offset_ += length;
		}
		else
		{
			for (auto o : obj)
			{
				write(o);
			}
		}
	}

	void write(const std::string& obj);

	void write_raw(const std::string& obj);

	bool read_size(uint32_t& obj);

	template <typename T>
	bool read(T& obj)
	{
		static_assert(std::is_arithmetic_v<T>);

		const uint32_t length = sizeof(T);

		if (length > UINT32_MAX - read_offset_)
			return false;

		const uint32_t final_offset = read_offset_ + length;
		if (buffer_.size() < final_offset)
			return false;

		std::memcpy(&obj, buffer_.data() + read_offset_, length);
		if (!Utils::is_little_endian)
		{
			boost::endian::endian_reverse_inplace(obj);
		}
		read_offset_ = final_offset;

		return true;
	}

	template <typename T>
	bool read(std::vector<T>& obj)
	{
		static_assert(std::is_arithmetic_v<T>);

		uint32_t size = 0;
		if (!read_size(size))
			return false;

		if constexpr (sizeof(T) > 1)
		{
			if (size > UINT32_MAX / sizeof(T))
				return false;
		}

		const uint32_t length = size * sizeof(T);

		if (length > UINT32_MAX - read_offset_)
			return false;

		const uint32_t final_offset = read_offset_ + length;
		if (buffer_.size() < final_offset)
			return false;

		if constexpr (sizeof(T) == 1)
		{
			obj.resize(size);
			std::memcpy(obj.data(), buffer_.data() + read_offset_, length);
			read_offset_ = final_offset;
		}
		else
		{
			obj.resize(size);
			for (uint32_t i = 0; i < size; i++)
			{
				if (!read(obj[i]))
					return false;
			}
		}

		return true;
	}

	bool read(std::string& obj);

	bool operator==(const BinaryBuffer& obj) const;

private:
	std::vector<uint8_t> buffer_;
	uint32_t write_offset_ = 0;
	uint32_t read_offset_ = 0;

	void grow_if_needed(uint32_t write_length);
};
