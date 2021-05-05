#include "pch.hpp"
#include "GetBlockMsg.hpp"

#include "Chain.hpp"
#include "InvMsg.hpp"
#include "Log.hpp"
#include "NetClient.hpp"

GetBlockMsg::GetBlockMsg(const std::string& fromBlockId)
	: FromBlockId(fromBlockId)
{
}

void GetBlockMsg::Handle(std::shared_ptr<Connection>& con)
{
	const auto& endpoint = con->Socket.remote_endpoint();
	LOG_TRACE("Recieved GetBlockMsg from {}:{}", endpoint.address().to_string(), endpoint.port());

	auto [block, height] = Chain::LocateBlockInActiveChain(FromBlockId);
	if (height == -1)
		height = 1;

	Chain::Mutex.lock();

	std::vector<std::shared_ptr<Block>> blocks;
	blocks.reserve(ChunkSize);

	int64_t max_height = height + ChunkSize;
	if (max_height > Chain::ActiveChain.size())
		max_height = Chain::ActiveChain.size();
	for (int64_t i = height; i < max_height - 1; i++)
		blocks.push_back(Chain::ActiveChain[i]);

	Chain::Mutex.unlock();

	LOG_TRACE("Sending {} blocks to {}:{}", blocks.size(), endpoint.address().to_string(), endpoint.port());
	NetClient::SendMsg(con, InvMsg(blocks));
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
