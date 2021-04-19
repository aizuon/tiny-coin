#include "pch.hpp"

#include "GetBlockMsg.hpp"
#include "InvMsg.hpp"
#include "NetClient.hpp"
#include "Chain.hpp"
#include "Log.hpp"

GetBlockMsg::GetBlockMsg(const std::string& fromBlockId)
	: FromBlockId(fromBlockId)
{

}

void GetBlockMsg::Handle(const std::shared_ptr<NetClient::Connection>& con)
{
	LOG_TRACE("Recieved GetBlockMsg from {}", con->Socket.remote_endpoint().address().to_string());

	auto [block, height] = Chain::LocateBlockInActiveChain(FromBlockId);
	if (height == -1)
		height = 1;

	Chain::Lock.lock();

	std::vector<std::shared_ptr<Block>> blocks;
	blocks.reserve(ChunkSize);

	int64_t max_height = height + ChunkSize;
	if (max_height > Chain::ActiveChain.size())
		max_height = Chain::ActiveChain.size();
	for (int64_t i = height; i < max_height - 1; i++)
		blocks.push_back(Chain::ActiveChain[i]);

	Chain::Lock.unlock();

	LOG_TRACE("Sending {} blocks to {}", blocks.size(), con->Socket.remote_endpoint().address().to_string());
	NetClient::SendMsgAsync(con, InvMsg(blocks));
}

BinaryBuffer GetBlockMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(FromBlockId);

	return buffer;
}

bool GetBlockMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	if (!buffer.Read(FromBlockId))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

Opcode GetBlockMsg::GetOpcode() const
{
	return Opcode::GetBlockMsg;
}
