#pragma once
#include "util/binary_buffer.hpp"

class IDeserializable
{
public:
	virtual bool deserialize(BinaryBuffer& buffer) = 0;

	virtual ~IDeserializable() = default;
};
