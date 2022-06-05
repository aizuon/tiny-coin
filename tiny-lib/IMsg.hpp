#pragma once
#include "IDeserializable.hpp"
#include "IHandleable.hpp"
#include "ISerializable.hpp"

class IMsg : public IHandleable, public ISerializable, public IDeserializable
{
public:
	~IMsg() override = default;
};
