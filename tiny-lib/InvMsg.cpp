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

void InvMsg::Handle(std::shared_ptr<Connection> con)
{
	LOG_INFO("Recieved initial sync from {}:{}", con->Socket.remote_endpoint().address().to_string(),
	         con->Socket.remote_endpoint().port());

	std::vector<std::shared_ptr<Block>> new_blocks;
	for (const auto& block : Blocks)
	{
		const auto [found_block, found_height, found_idx] = Chain::LocateBlockInAllChains(block->Id());
		if (found_block == nullptr)
		{
			new_blocks.push_back(block);
		}
	}

	if (new_blocks.empty())
	{
		LOG_INFO("Initial block download complete");

		Chain::InitialBlockDownloadComplete = true;
		Chain::SaveToDisk();

		return;
	}

	for (const auto& new_block : new_blocks)
		Chain::ConnectBlock(new_block);

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

	uint32_t blocks_size = 0;
	if (!buffer.ReadSize(blocks_size))
	{
		*this = std::move(copy);

		return false;
	}
	Blocks.reserve(blocks_size);
	for (uint32_t i = 0; i < blocks_size; i++)
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
