#include "pch.hpp"

#include "SendActiveChainMsg.hpp"
#include "Chain.hpp"
#include "MsgCache.hpp"

void SendActiveChainMsg::Handle(std::shared_ptr<NetClient::Connection>& con)
{
	MsgCache::SendActiveChainMsg = std::make_shared<SendActiveChainMsg>(*this);
}

BinaryBuffer SendActiveChainMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(Chain::ActiveChain.size());
	for (const auto& block : Chain::ActiveChain)
	{
		buffer.WriteRaw(block->Serialize().GetBuffer());
	}

	return buffer;
}

bool SendActiveChainMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	size_t activeChainSize = 0;
	if (!buffer.Read(activeChainSize))
	{
		*this = std::move(copy);

		return false;
	}
	ActiveChain = std::vector<std::shared_ptr<Block>>();
	ActiveChain.reserve(activeChainSize);
	for (size_t i = 0; i < activeChainSize; i++)
	{
		auto block = std::make_shared<Block>();
		if (!block->Deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		ActiveChain.push_back(block);
	}

	return true;
}

Opcode SendActiveChainMsg::GetOpcode() const
{
	return Opcode::SendActiveChainMsg;
}
