#pragma once
#include "NetClient.hpp"
#include "Opcodes.hpp"

class IHandleable
{
public:
	virtual void Handle(const std::shared_ptr<NetClient::Connection>& con) const = 0;

	virtual Opcode GetOpcode() const = 0;

	virtual ~IHandleable() {};
};
