#include "pch.hpp"

#include "BlockInfoMsg.hpp"
#include "Chain.hpp"
#include "Log.hpp"

BlockInfoMsg::BlockInfoMsg(const std::shared_ptr<::Block>& block)
	: Block(block)
{

}

void BlockInfoMsg::Handle(const std::shared_ptr<NetClient::Connection>& con)
{
	LOG_INFO("Recieved block {} from peer {}", Block->Id(), con->Socket.remote_endpoint().address().to_string());

	Chain::ConnectBlock(Block);
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
