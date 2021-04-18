#include "pch.hpp"

#include "GetActiveChainMsg.hpp"
#include "BinaryBuffer.hpp"
#include "Block.hpp"

GetActiveChainMsg::GetActiveChainMsg(const std::vector<std::shared_ptr<Block>>& activeChain)
	: ActiveChain(activeChain)
{

}

void GetActiveChainMsg::Handle(NetClient::ConnectionHandle con_handle) const
{
	//TODO
}

BinaryBuffer GetActiveChainMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(ActiveChain.size());
	for (const auto& block : ActiveChain)
	{
		buffer.WriteRaw(block->Serialize().GetBuffer());
	}

	return buffer;
}

bool GetActiveChainMsg::Deserialize(BinaryBuffer& buffer)
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
		auto vec_it = std::find_if(ActiveChain.begin(), ActiveChain.end(),
			[&block](const std::shared_ptr<Block>& o)
			{
				return block->Id() == o->Id();
			});
		if (vec_it == ActiveChain.end())
		{
			ActiveChain.push_back(block);
		}
	}

	return true;
}
