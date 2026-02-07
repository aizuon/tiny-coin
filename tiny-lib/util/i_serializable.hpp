#pragma once
#include "util/binary_buffer.hpp"

class ISerializable
{
public:
	virtual BinaryBuffer serialize() const = 0;

	virtual ~ISerializable() = default;
};
