#include "pch.hpp"

#include "InvMsg.hpp"
#include "BinaryBuffer.hpp"

InvMsg::InvMsg(const std::vector<std::string>& blocks)
	: Blocks(blocks)
{

}

void InvMsg::Handle(NetClient::ConnectionHandle con_handle) const
{
	//TODO
}

BinaryBuffer InvMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(Blocks.size());
	for (const auto& block : Blocks)
	{
		buffer.Write(block);
	}

	return buffer;
}

bool InvMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	size_t blocksSize = 0;
	if (!buffer.Read(blocksSize))
	{
		*this = std::move(copy);

		return false;
	}
	Blocks = std::vector<std::string>();
	Blocks.reserve(blocksSize);
	for (size_t i = 0; i < blocksSize; i++)
	{
		std::string block;
		if (!buffer.Read(block))
		{
			*this = std::move(copy);

			return false;
		}
		Blocks.push_back(block);
	}

	return true;
}
