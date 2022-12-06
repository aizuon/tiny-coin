#include "pch.hpp"
#include "SendActiveChainMsg.hpp"

#include "Chain.hpp"
#include "MsgCache.hpp"

void SendActiveChainMsg::Handle(std::shared_ptr<Connection> con)
{
	MsgCache::SendActiveChainMsg = std::make_shared<SendActiveChainMsg>(*this);
}

BinaryBuffer SendActiveChainMsg::Serialize() const
{
	BinaryBuffer buffer;

	{
		std::scoped_lock lock(Chain::Mutex);

		buffer.WriteSize(Chain::ActiveChain.size());
		for (const auto& block : Chain::ActiveChain)
		{
			buffer.WriteRaw(block->Serialize().GetBuffer());
		}
	}

	return buffer;
}

bool SendActiveChainMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	uint32_t active_chain_size = 0;
	if (!buffer.ReadSize(active_chain_size))
	{
		*this = std::move(copy);

		return false;
	}
	if (!ActiveChain.empty())
		ActiveChain.clear();
	ActiveChain.reserve(active_chain_size);
	for (uint32_t i = 0; i < active_chain_size; i++)
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
