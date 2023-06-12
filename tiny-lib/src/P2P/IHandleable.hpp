#pragma once
#include <memory>

#include "P2P/Connection.hpp"
#include "P2P/Opcodes.hpp"

class IHandleable
{
public:
	virtual void Handle(std::shared_ptr<Connection> con) = 0;

	virtual Opcode GetOpcode() const = 0;

	virtual ~IHandleable() = default;
};
