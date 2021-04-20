#pragma once
#include "IDeserializable.hpp"
#include "ISerializable.hpp"
#include "IHandleable.hpp"

class IMsg : public virtual IHandleable, public virtual ISerializable, public virtual IDeserializable
{
public:
	virtual ~IMsg() {};
};
