#pragma once
#include "NetClient.hpp"

class IHandleable
{
public:
	virtual void Handle(NetClient::ConnectionHandle con_handle) const = 0;

	virtual ~IHandleable() {};
};