#include "net/send_active_chain_msg.hpp"

#include "core/chain.hpp"
#include "net/msg_cache.hpp"

void SendActiveChainMsg::handle([[maybe_unused]] const std::shared_ptr<Connection>& con)
{
	MsgCache::set_send_active_chain_msg(std::make_shared<SendActiveChainMsg>(*this));
}

BinaryBuffer SendActiveChainMsg::serialize() const
{
	std::vector<std::shared_ptr<Block>> chain_snapshot;
	{
		std::scoped_lock lock(Chain::mutex);
		chain_snapshot = Chain::active_chain;
	}

	BinaryBuffer buffer;
	buffer.write_size(static_cast<uint32_t>(chain_snapshot.size()));
	for (const auto& block : chain_snapshot)
	{
		buffer.write_raw(block->serialize().get_buffer());
	}

	return buffer;
}

bool SendActiveChainMsg::deserialize(BinaryBuffer& buffer)
{
	uint32_t active_chain_size = 0;
	if (!buffer.read_size(active_chain_size))
		return false;

	std::vector<std::shared_ptr<Block>> new_active_chain;
	new_active_chain.reserve(active_chain_size);
	for (uint32_t i = 0; i < active_chain_size; i++)
	{
		auto block = std::make_shared<Block>();
		if (!block->deserialize(buffer))
			return false;
		new_active_chain.push_back(std::move(block));
	}

	active_chain = std::move(new_active_chain);

	return true;
}

Opcode SendActiveChainMsg::get_opcode() const
{
	return Opcode::SendActiveChainMsg;
}
