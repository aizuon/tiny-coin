#pragma once
#include <memory>

#include "net/connection.hpp"
#include "net/opcodes.hpp"

class IHandleable
{
public:
	virtual void handle(const std::shared_ptr<Connection>& con) = 0;

	virtual Opcode get_opcode() const = 0;

	virtual ~IHandleable() = default;
};
