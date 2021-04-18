#pragma once
#include "NetClient.hpp"
#include "Opcodes.hpp"

class IHandleable
{
public:
	virtual void Handle(NetClient::ConnectionHandle con_handle) const = 0;

	virtual Opcode GetOpcode() const = 0;

	virtual ~IHandleable() {};
};
