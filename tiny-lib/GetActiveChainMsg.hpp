#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "IHandleable.hpp"
#include "ISerializable.hpp"
#include "IDeserializable.hpp"

class Block;

class GetActiveChainMsg : public IHandleable, public ISerializable, public IDeserializable
{
public:
	GetActiveChainMsg() = default;
	GetActiveChainMsg(const std::vector<std::shared_ptr<Block>>& activeChain);

	std::vector<std::shared_ptr<Block>> ActiveChain;

	void Handle(NetClient::ConnectionHandle con_handle) const override;
	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

private:

};
