#pragma once
#include <cassert>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
#include <boost/endian/conversion.hpp>

#include "Utils.hpp"

class BinaryBuffer
{
public:
	BinaryBuffer() = default;
	BinaryBuffer(const std::vector<uint8_t>& obj);
	BinaryBuffer(std::vector<uint8_t>&& obj);

	BinaryBuffer(const BinaryBuffer& obj);
	BinaryBuffer(BinaryBuffer&& obj) noexcept;
	BinaryBuffer& operator=(const BinaryBuffer& obj);
	BinaryBuffer& operator=(BinaryBuffer&& obj) noexcept;

	inline const std::vector<uint8_t>& GetBuffer() const
	{
		return Buffer;
	}

	inline std::vector<uint8_t>& GetWritableBuffer()
	{
		return Buffer;
	}

	inline uint32_t GetSize() const
	{
		return Buffer.size();
	}

	inline uint32_t GetWriteOffset() const
	{
		return WriteOffset;
	}

	inline uint32_t GetReadOffset() const
	{
		return ReadOffset;
	}

	inline void GrowTo(uint32_t size)
	{
		assert(size > Buffer.size());

		Buffer.resize(size);
	}

	inline void Reserve(uint32_t size)
	{
		Buffer.reserve(size);
	}

	void WriteSize(uint32_t obj);

	template <typename T>
	void Write(T obj)
	{
		static_assert(std::is_arithmetic_v<T>);

		std::scoped_lock lock(Mutex);

		if (!Utils::IsLittleEndian)
		{
			boost::endian::endian_reverse_inplace(obj);
		}

		const uint32_t length = sizeof(T);
		GrowIfNeeded(length);
		std::memcpy(Buffer.data() + WriteOffset, &obj, length);
		WriteOffset += length;
	}

	template <typename T>
	void Write(const std::vector<T>& obj)
	{
		std::scoped_lock lock(Mutex);

		const uint32_t size = obj.size();
		WriteSize(size);

		const uint32_t length = size * sizeof(T);
		GrowIfNeeded(length);
		for (auto o : obj)
		{
			Write(o);
		}
	}

	template <typename T>
	void WriteRaw(const std::vector<T>& obj)
	{
		std::scoped_lock lock(Mutex);

		const uint32_t length = obj.size() * sizeof(T);
		GrowIfNeeded(length);
		for (auto o : obj)
		{
			Write(o);
		}
	}

	void Write(const std::string& obj);

	void WriteRaw(const std::string& obj);

	bool ReadSize(uint32_t& obj);

	template <typename T>
	bool Read(T& obj)
	{
		static_assert(std::is_arithmetic_v<T>);

		std::scoped_lock lock(Mutex);

		const uint32_t length = sizeof(T);

		const uint32_t final_offset = ReadOffset + length;
		if (Buffer.size() < final_offset)
			return false;

		std::memcpy(&obj, Buffer.data() + ReadOffset, length);
		if (!Utils::IsLittleEndian)
		{
			boost::endian::endian_reverse_inplace(obj);
		}
		ReadOffset = final_offset;

		return true;
	}

	template <typename T>
	bool Read(std::vector<T>& obj)
	{
		std::scoped_lock lock(Mutex);

		uint32_t size = 0;
		if (!ReadSize(size))
			return false;

		const uint32_t length = size * sizeof(T);

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

	bool Read(std::string& obj);

	bool operator==(const BinaryBuffer& obj) const;

private:
	std::vector<uint8_t> Buffer;
	uint32_t WriteOffset = 0;
	uint32_t ReadOffset = 0;

	std::recursive_mutex Mutex;

	static constexpr float BUFFER_GROW_FACTOR = 1.5f;

	void GrowIfNeeded(uint32_t write_length);
};
