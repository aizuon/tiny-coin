#include "pch.hpp"

#include "BlockInfoMsg.hpp"

BlockInfoMsg::BlockInfoMsg(const std::shared_ptr<::Block>& block)
	: Block(block)
{

}

void BlockInfoMsg::Handle(const std::shared_ptr<NetClient::Connection>& con) const
{
	//TODO
}

BinaryBuffer BlockInfoMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.WriteRaw(Block->Serialize().GetBuffer());

	return buffer;
}

bool BlockInfoMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	auto block = std::make_shared<::Block>();
	if (!block->Deserialize(buffer))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

Opcode BlockInfoMsg::GetOpcode() const
{
	return Opcode::BlockInfoMsg;
}
