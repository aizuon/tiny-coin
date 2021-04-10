#pragma once
#include <cstdint>
#include <vector>

class ISerializable
{
public:
	virtual std::vector<uint8_t> Serialize() const = 0;

	virtual ~ISerializable() {};
};