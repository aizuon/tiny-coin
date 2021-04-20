#include "pch.hpp"

#include "BlockInfoMsg.hpp"
#include "Log.hpp"
#include "Chain.hpp"

BlockInfoMsg::BlockInfoMsg(const std::shared_ptr<::Block>& block)
	: Block(block)
{

}

void BlockInfoMsg::Handle(std::shared_ptr<NetClient::Connection>& con)
{
	LOG_INFO("Recieved block {} from peer {}:{}", Block->Id(), con->Socket.remote_endpoint().address().to_string(), con->Socket.remote_endpoint().port());

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
