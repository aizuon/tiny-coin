#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "IHandleable.hpp"
#include "ISerializable.hpp"
#include "IDeserializable.hpp"

class GetMempoolMsg : public IHandleable, public ISerializable, public IDeserializable
{
public:
	GetMempoolMsg() = default;
	GetMempoolMsg(const std::vector<std::string>& mempool);

	std::vector<std::string> Mempool;

	void Handle(NetClient::ConnectionHandle con_handle) const override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

private:

};
