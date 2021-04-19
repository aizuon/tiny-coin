#include "pch.hpp"

#include "InvMsg.hpp"
#include "GetBlockMsg.hpp"
#include "NetClient.hpp"
#include "Chain.hpp"
#include "Log.hpp"

InvMsg::InvMsg(const std::vector<std::shared_ptr<Block>>& blocks)
	: Blocks(blocks)
{

}

void InvMsg::Handle(const std::shared_ptr<NetClient::Connection>& con)
{
	LOG_INFO("Recieved initial sync from {}", con->Socket.remote_endpoint().address().to_string());

	std::vector<std::shared_ptr<Block>> newBlocks;
	for (const auto& block : Blocks)
	{
		auto [found_block, found_height, found_idx] = Chain::LocateBlockInAllChains(block->Id());
		if (found_block == nullptr)
		{
			newBlocks.push_back(block);
		}
	}

	if (newBlocks.empty())
	{
		LOG_INFO("Initial block download complete");

		Chain::InitialBlockDownloadComplete = true;

		return;
	}

	for (const auto& newBlock : newBlocks)
		Chain::ConnectBlock(newBlock);

	auto new_tip_id = Chain::ActiveChain.back()->Id();
	LOG_INFO("Continuing initial sync from {}", new_tip_id);

	NetClient::SendMsgAsync(con, GetBlockMsg(new_tip_id));
}

BinaryBuffer InvMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(Blocks.size());
	for (const auto& block : Blocks)
	{
		buffer.WriteRaw(block->Serialize().GetBuffer());
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
	Blocks = std::vector<std::shared_ptr<Block>>();
	Blocks.reserve(blocksSize);
	for (size_t i = 0; i < blocksSize; i++)
	{
		auto block = std::make_shared<Block>();
		if (!block->Deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		Blocks.push_back(block);
	}

	return true;
}

Opcode InvMsg::GetOpcode() const
{
	return Opcode::InvMsg;
}