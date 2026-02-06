#include "net/block_info_msg.hpp"

#include "core/chain.hpp"
#include "util/log.hpp"

BlockInfoMsg::BlockInfoMsg(const std::shared_ptr<::Block>& block)
	: block(block)
{}

void BlockInfoMsg::handle(const std::shared_ptr<Connection>& con)
{
	LOG_INFO("Recieved block {} from peer {}:{}", block->id(), con->socket.remote_endpoint().address().to_string(),
		con->socket.remote_endpoint().port());

	Chain::connect_block(block);
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
	auto copy = *this;

	block = std::make_shared<::Block>();
	if (!block->deserialize(buffer))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

Opcode BlockInfoMsg::get_opcode() const
{
	return Opcode::BlockInfoMsg;
}
