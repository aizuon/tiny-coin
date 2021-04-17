#pragma once
#include <cstdint>
#include <vector>

class BinaryBuffer;

class IDeserializable
{
public:
	virtual bool Deserialize(BinaryBuffer& buffer) = 0;

	virtual ~IDeserializable() {};
};