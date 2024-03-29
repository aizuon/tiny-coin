#include "pch.hpp"
#include "BlockInfoMsg.hpp"

#include "Chain.hpp"
#include "Log.hpp"

BlockInfoMsg::BlockInfoMsg(const std::shared_ptr<::Block>& block)
	: Block(block)
{
}

void BlockInfoMsg::Handle(std::shared_ptr<Connection> con)
{
	LOG_INFO("Recieved block {} from peer {}:{}", Block->Id(), con->Socket.remote_endpoint().address().to_string(),
	         con->Socket.remote_endpoint().port());

	Chain::ConnectBlock(Block);
	Chain::SaveToDisk();
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

	Block = std::make_shared<::Block>();
	if (!Block->Deserialize(buffer))
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
