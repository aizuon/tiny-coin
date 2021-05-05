#pragma once
#include "BinaryBuffer.hpp"

class IDeserializable
{
public:
	virtual bool Deserialize(BinaryBuffer& buffer) = 0;

	virtual ~IDeserializable()
	{
	};
};
