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
	LOG_INFO("Recieved initial sync from {}:{}", con->socket.remote_endpoint().address().to_string(),
		con->socket.remote_endpoint().port());

	std::vector<std::shared_ptr<Block>> new_blocks;
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

	for (const auto& new_block : new_blocks)
		Chain::connect_block(new_block);

	std::string new_tip_id;
	{
		std::scoped_lock lock(Chain::mutex);
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
	auto copy = *this;

	uint32_t blocks_size = 0;
	if (!buffer.read_size(blocks_size))
	{
		*this = std::move(copy);

		return false;
	}
	blocks.reserve(blocks_size);
	for (uint32_t i = 0; i < blocks_size; i++)
	{
		auto block = std::make_shared<Block>();
		if (!block->deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		blocks.push_back(block);
	}

	return true;
}

Opcode InvMsg::get_opcode() const
{
	return Opcode::InvMsg;
}
