#pragma once
#include <cstdint>
#include <string>

#include "IHandleable.hpp"
#include "ISerializable.hpp"
#include "IDeserializable.hpp"

class GetBlockMsg : public IHandleable, public ISerializable, public IDeserializable
{
public:
	GetBlockMsg() = default;
	GetBlockMsg(const std::string& fromBlockId);

	std::string FromBlockId;

	void Handle(NetClient::ConnectionHandle con_handle) const override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

private:
	static constexpr uint64_t ChunkSize = 50;
};