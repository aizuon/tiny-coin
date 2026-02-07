#include "net/inv_msg.hpp"

#include "core/chain.hpp"
#include "net/get_block_msg.hpp"
#include "util/log.hpp"
#include "net/net_client.hpp"

InvMsg::InvMsg(const std::vector<std::shared_ptr<Block>>& blocks)
	: blocks(blocks)
{}

void InvMsg::handle(const std::shared_ptr<Connection>& con)
{
	const auto endpoint = con->socket.remote_endpoint();
	LOG_INFO("Received initial sync from {}:{}", endpoint.address().to_string(),
		endpoint.port());

	std::vector<std::shared_ptr<Block>> new_blocks;
	new_blocks.reserve(blocks.size());
	for (const auto& block : blocks)
	{
		const auto [found_block, found_height, found_idx] = Chain::locate_block_in_all_chains(block->id());
		if (found_block == nullptr)
		{
			new_blocks.push_back(block);
		}
	}

	if (new_blocks.empty())
	{
		LOG_INFO("Initial block download complete");

		Chain::initial_block_download_complete = true;
		Chain::save_to_disk();

		return;
	}

	bool any_connected = false;
	for (const auto& new_block : new_blocks)
	{
		if (Chain::connect_block(new_block) >= 0)
			any_connected = true;
	}

	if (any_connected)
		Chain::save_to_disk();

	std::string new_tip_id;
	{
		std::scoped_lock lock(Chain::mutex);
		if (Chain::active_chain.empty())
			return;
		new_tip_id = Chain::active_chain.back()->id();
	}
	LOG_INFO("Continuing initial sync from {}", new_tip_id);

	NetClient::send_msg(con, GetBlockMsg(new_tip_id));
}

BinaryBuffer InvMsg::serialize() const
{
	BinaryBuffer buffer;

	buffer.write_size(static_cast<uint32_t>(blocks.size()));
	for (const auto& block : blocks)
	{
		buffer.write_raw(block->serialize().get_buffer());
	}

	return buffer;
}

bool InvMsg::deserialize(BinaryBuffer& buffer)
{
	uint32_t blocks_size = 0;
	if (!buffer.read_size(blocks_size))
		return false;

	std::vector<std::shared_ptr<Block>> new_blocks;
	new_blocks.reserve(blocks_size);
	for (uint32_t i = 0; i < blocks_size; i++)
	{
		auto block = std::make_shared<Block>();
		if (!block->deserialize(buffer))
			return false;
		new_blocks.push_back(std::move(block));
	}

	blocks = std::move(new_blocks);

	return true;
}

Opcode InvMsg::get_opcode() const
{
	return Opcode::InvMsg;
}
