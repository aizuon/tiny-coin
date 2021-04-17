#pragma once
#include <cstdint>
#include <vector>

class BinaryBuffer;

class ISerializable
{
public:
	virtual BinaryBuffer Serialize() const = 0;

	virtual ~ISerializable() {};
};