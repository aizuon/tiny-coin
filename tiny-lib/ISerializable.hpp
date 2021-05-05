#pragma once
#include "BinaryBuffer.hpp"

class ISerializable
{
public:
	virtual BinaryBuffer Serialize() const = 0;

	virtual ~ISerializable()
	{
	}
};
