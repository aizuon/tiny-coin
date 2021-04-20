#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <mutex>

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

	inline size_t GetLength() const
	{
		return Buffer.size();
	}

	inline size_t GetWriteOffset() const
	{
		return WriteOffset;
	}

	inline size_t GetReadOffset() const
	{
		return ReadOffset;
	}

	inline void GrowTo(size_t size)
	{
		if (size < Buffer.size())
			return;

		Buffer.resize(size);
	}

	inline void Reserve(size_t size)
	{
		Buffer.reserve(size);
	}

    template<typename T>
    void Write(T obj)
    {
		std::lock_guard<std::mutex> lock(Mutex);

		static_assert(std::is_trivial_v<T>);

		size_t length = sizeof(T);

		size_t finalLength = WriteOffset + length;
		GrowIfNeeded(finalLength);

		memcpy(Buffer.data() + WriteOffset, &obj, length);
		WriteOffset = finalLength;
    }

	template<typename T>
	void Write(const std::vector<T>& obj)
	{
		std::lock_guard<std::mutex> lock(Mutex);

		static_assert(std::is_trivial_v<T>);

		size_t size = obj.size();
		size_t length1 = sizeof(size);
		size_t length2 = size * sizeof(T);

		size_t finalLength = WriteOffset + length1 + length2;
		GrowIfNeeded(finalLength);

		memcpy(Buffer.data() + WriteOffset, &size, length1);
		memcpy(Buffer.data() + WriteOffset + length1, obj.data(), length2);
		WriteOffset = finalLength;
	}

	template<typename T>
	void WriteRaw(const std::vector<T>& obj)
	{
		std::lock_guard<std::mutex> lock(Mutex);

		static_assert(std::is_trivial_v<T>);

		size_t length = obj.size() * sizeof(T);

		size_t finalLength = WriteOffset + length;
		GrowIfNeeded(finalLength);

		memcpy(Buffer.data() + WriteOffset, obj.data(), length);
		WriteOffset = finalLength;
	}

	void Write(const std::string& obj);

	void WriteRaw(const std::string& obj);

	template<typename T>
	bool Read(T& obj)
	{
		std::lock_guard<std::mutex> lock(Mutex);

		static_assert(std::is_trivial_v<T>);

		size_t length = sizeof(T);

		size_t finalOffset = ReadOffset + length;
		if (Buffer.size() < finalOffset)
			return false;

		memcpy(&obj, Buffer.data() + ReadOffset, length);
		ReadOffset = finalOffset;

		return true;
	}

	template<typename T>
	bool Read(std::vector<T>& obj)
	{
		std::lock_guard<std::mutex> lock(Mutex);

		static_assert(std::is_trivial_v<T>);

		size_t size = 0;
		size_t length1 = sizeof(size);
		if (Buffer.size() < ReadOffset + length1)
			return false;

		memcpy(&size, Buffer.data() + ReadOffset, length1);

		size_t length2 = size * sizeof(T);

		size_t finalOffset = ReadOffset + length1 + length2;
		if (Buffer.size() < finalOffset)
			return false;

		obj.resize(size);
		memcpy(obj.data(), Buffer.data() + ReadOffset + length1, length2);
		ReadOffset = finalOffset;

		return true;
	}

	bool Read(std::string& obj);

	bool operator==(const BinaryBuffer& obj) const;

private:
	std::vector<uint8_t> Buffer;
	size_t WriteOffset = 0;
	size_t ReadOffset = 0;

	std::mutex Mutex;

	static constexpr float BUFFER_GROW_FACTOR = 1.5f;

	void GrowIfNeeded(size_t finalLength);
};