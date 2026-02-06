#include "net/send_active_chain_msg.hpp"

#include "core/chain.hpp"
#include "net/msg_cache.hpp"

void SendActiveChainMsg::handle([[maybe_unused]] const std::shared_ptr<Connection>& con)
{
	MsgCache::send_active_chain_msg = std::make_shared<SendActiveChainMsg>(*this);
}

BinaryBuffer SendActiveChainMsg::serialize() const
{
	BinaryBuffer buffer;

	{
		std::scoped_lock lock(Chain::mutex);

		buffer.write_size(static_cast<uint32_t>(Chain::active_chain.size()));
		for (const auto& block : Chain::active_chain)
		{
			buffer.write_raw(block->serialize().get_buffer());
		}
	}

	return buffer;
}

bool SendActiveChainMsg::deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	uint32_t active_chain_size = 0;
	if (!buffer.read_size(active_chain_size))
	{
		*this = std::move(copy);

		return false;
	}
	active_chain.clear();
	active_chain.reserve(active_chain_size);
	for (uint32_t i = 0; i < active_chain_size; i++)
	{
		auto block = std::make_shared<Block>();
		if (!block->deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		active_chain.push_back(block);
	}

	return true;
}

Opcode SendActiveChainMsg::get_opcode() const
{
	return Opcode::SendActiveChainMsg;
}
