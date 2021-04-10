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
	BinaryBuffer(size_t length);

    inline const std::vector<uint8_t>& GetBuffer() const
    {
        return buffer;
    }

	inline size_t GetLength() const
	{
		return buffer.size();
	}

	inline size_t GetWriteOffset() const
	{
		return writeOffset;
	}

	inline size_t GetReadOffset() const
	{
		return readOffset;
	}

    template<typename T>
    void Write(T obj)
    {
		std::lock_guard<std::mutex> lock(mtx);

		static_assert(std::is_trivial_v<T>);

		size_t length = sizeof(T);

		size_t finalLength = writeOffset + length;
		GrowIfNeeded(finalLength);

		memcpy(buffer.data() + writeOffset, &obj, length);
		writeOffset = finalLength;
    }

	template<typename T>
	void Write(const std::vector<T>& obj)
	{
		std::lock_guard<std::mutex> lock(mtx);

		static_assert(std::is_trivial_v<T>);

		size_t size = obj.size();
		size_t length1 = sizeof(size);
		size_t length2 = size * sizeof(T);

		size_t finalLength = writeOffset + length1 + length2;
		GrowIfNeeded(finalLength);

		memcpy(buffer.data() + writeOffset, &size, length1);
		memcpy(buffer.data() + writeOffset + length1, obj.data(), length2);
		writeOffset = finalLength;
	}

	void Write(const std::string& obj);

	template<typename T>
	bool Read(T& obj)
	{
		std::lock_guard<std::mutex> lock(mtx);

		static_assert(std::is_trivial_v<T>);

		size_t length = sizeof(T);

		size_t finalOffset = readOffset + length;
		if (buffer.size() < finalOffset)
			return false;

		memcpy(&obj, buffer.data() + readOffset, length);
		readOffset = finalOffset;

		return true;
	}

	template<typename T>
	bool Read(std::vector<T>& obj)
	{
		std::lock_guard<std::mutex> lock(mtx);

		static_assert(std::is_trivial_v<T>);

		size_t size = 0;
		size_t length1 = sizeof(size);
		if (buffer.size() < readOffset + length1)
			return false;

		memcpy(&size, buffer.data() + readOffset, length1);

		size_t length2 = size * sizeof(T);

		size_t finalOffset = readOffset + length1 + length2;
		if (buffer.size() < finalOffset)
			return false;

		obj.resize(size);
		memcpy(obj.data(), buffer.data() + readOffset + length1, length2);
		readOffset = finalOffset;

		return true;
	}

	bool Read(std::string& obj);

private:
	std::vector<uint8_t> buffer;
	size_t writeOffset = 0;
	size_t readOffset = 0;

	std::mutex mtx;

	static constexpr float BUFFER_GROW_FACTOR = 1.5f;

	void GrowIfNeeded(size_t finalLength);
};