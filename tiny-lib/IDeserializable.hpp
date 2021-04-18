#pragma once
#include <cstdint>
#include <vector>

#include "BinaryBuffer.hpp"

class IDeserializable
{
public:
	virtual bool Deserialize(BinaryBuffer& buffer) = 0;

	virtual ~IDeserializable() {};
};