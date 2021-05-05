#pragma once
#include "IDeserializable.hpp"
#include "IHandleable.hpp"
#include "ISerializable.hpp"

class IMsg : public virtual IHandleable, public virtual ISerializable, public virtual IDeserializable
{
public:
	virtual ~IMsg()
	{
	}
};
