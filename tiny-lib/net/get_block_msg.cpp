#include "net/get_block_msg.hpp"

#include <algorithm>

#include "core/chain.hpp"
#include "net/inv_msg.hpp"
#include "util/log.hpp"
#include "net/net_client.hpp"

GetBlockMsg::GetBlockMsg(const std::string& from_block_id)
	: from_block_id(from_block_id)
{}

void GetBlockMsg::handle(const std::shared_ptr<Connection>& con)
{
	const auto& endpoint = con->socket.remote_endpoint();
	LOG_TRACE("Received GetBlockMsg from {}:{}", endpoint.address().to_string(), endpoint.port());

	auto [block, height] = Chain::locate_block_in_active_chain(from_block_id);
	if (height == -1)
		height = 1;
	else
		height += 1;

	std::vector<std::shared_ptr<Block>> blocks;
	blocks.reserve(CHUNK_SIZE);

	{
		std::scoped_lock lock(Chain::mutex);

		const auto chain_size = static_cast<int64_t>(Chain::active_chain.size());
		const int64_t max_height = std::min(height + static_cast<int64_t>(CHUNK_SIZE), chain_size);
		for (int64_t i = height; i < max_height; i++)
			blocks.push_back(Chain::active_chain[i]);
	}

	LOG_TRACE("Sending {} block(s) to {}:{}", blocks.size(), endpoint.address().to_string(), endpoint.port());
	NetClient::send_msg(con, InvMsg(blocks));
}

BinaryBuffer GetBlockMsg::serialize() const
{
	BinaryBuffer buffer;

	buffer.write(from_block_id);

	return buffer;
}

bool GetBlockMsg::deserialize(BinaryBuffer& buffer)
{
	std::string new_from_block_id;
	if (!buffer.read(new_from_block_id))
		return false;

	from_block_id = std::move(new_from_block_id);

	return true;
}

Opcode GetBlockMsg::get_opcode() const
{
	return Opcode::GetBlockMsg;
}
