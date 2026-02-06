#include "net/block_info_msg.hpp"

#include "core/chain.hpp"
#include "util/log.hpp"

BlockInfoMsg::BlockInfoMsg(const std::shared_ptr<::Block>& block)
	: block(block)
{}

void BlockInfoMsg::handle(const std::shared_ptr<Connection>& con)
{
	const auto endpoint = con->socket.remote_endpoint();
	LOG_INFO("Received block {} from peer {}:{}", block->id(), endpoint.address().to_string(),
		endpoint.port());

	if (Chain::connect_block(block) >= 0)
		Chain::save_to_disk();
}

BinaryBuffer BlockInfoMsg::serialize() const
{
	BinaryBuffer buffer;

	buffer.write_raw(block->serialize().get_buffer());

	return buffer;
}

bool BlockInfoMsg::deserialize(BinaryBuffer& buffer)
{
	auto new_block = std::make_shared<::Block>();
	if (!new_block->deserialize(buffer))
		return false;

	block = std::move(new_block);

	return true;
}

Opcode BlockInfoMsg::get_opcode() const
{
	return Opcode::BlockInfoMsg;
}
