#pragma once
#include "IDeserializable.hpp"
#include "P2P/IHandleable.hpp"
#include "ISerializable.hpp"

class IMsg : public IHandleable, public ISerializable, public IDeserializable
{
public:
	~IMsg() override = default;
};
