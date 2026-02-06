#pragma once
#include "util/i_deserializable.hpp"
#include "net/i_handleable.hpp"
#include "util/i_serializable.hpp"

class IMsg : public IHandleable, public ISerializable, public IDeserializable
{
public:
	~IMsg() override = default;
};
