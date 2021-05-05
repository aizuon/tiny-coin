#include "pch.hpp"
#include "InvMsg.hpp"

#include "Chain.hpp"
#include "GetBlockMsg.hpp"
#include "Log.hpp"
#include "NetClient.hpp"

InvMsg::InvMsg(const std::vector<std::shared_ptr<Block>>& blocks)
	: Blocks(blocks)
{
}

void InvMsg::Handle(std::shared_ptr<Connection>& con)
{
	LOG_INFO("Recieved initial sync from {}:{}", con->Socket.remote_endpoint().address().to_string(),
	         con->Socket.remote_endpoint().port());

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

	NetClient::SendMsg(con, GetBlockMsg(new_tip_id));
}

BinaryBuffer InvMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.WriteSize(Blocks.size());
	for (const auto& block : Blocks)
	{
		buffer.WriteRaw(block->Serialize().GetBuffer());
	}

	return buffer;
}

bool InvMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	uint32_t blocksSize = 0;
	if (!buffer.ReadSize(blocksSize))
	{
		*this = std::move(copy);

		return false;
	}
	Blocks = std::vector<std::shared_ptr<Block>>();
	Blocks.reserve(blocksSize);
	for (uint32_t i = 0; i < blocksSize; i++)
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
