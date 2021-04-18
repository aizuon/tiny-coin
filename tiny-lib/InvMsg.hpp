#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "IHandleable.hpp"
#include "ISerializable.hpp"
#include "IDeserializable.hpp"

class InvMsg : public IHandleable, public ISerializable, public IDeserializable
{
public:
	InvMsg() = default;
	InvMsg(const std::vector<std::string>& blocks);

	std::vector<std::string> Blocks;

	void Handle(NetClient::ConnectionHandle con_handle) const override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

private:

};
