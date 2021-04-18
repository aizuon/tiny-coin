#pragma once
#include "IHandleable.hpp"
#include "ISerializable.hpp"
#include "IDeserializable.hpp"

class IMsg : public virtual IHandleable, public virtual ISerializable, public virtual IDeserializable
{
public:
	virtual ~IMsg() {};
};
