#pragma once
#include "NetClient.hpp"
#include "Opcodes.hpp"

class IHandleable
{
public:
	virtual void Handle(std::shared_ptr<NetClient::Connection>& con) = 0;

	virtual Opcode GetOpcode() const = 0;

	virtual ~IHandleable() {};
};
