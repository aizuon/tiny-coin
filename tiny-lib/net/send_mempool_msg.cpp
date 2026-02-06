#include "net/send_mempool_msg.hpp"

#include <ranges>

#include "core/mempool.hpp"
#include "net/msg_cache.hpp"

void SendMempoolMsg::handle([[maybe_unused]] const std::shared_ptr<Connection>& con)
{
	MsgCache::send_mempool_msg = std::make_shared<SendMempoolMsg>(*this);
}

BinaryBuffer SendMempoolMsg::serialize() const
{
	BinaryBuffer buffer;

	{
		std::scoped_lock lock(Mempool::mutex);

		buffer.write_size(static_cast<uint32_t>(Mempool::map.size()));
		for (const auto& key : Mempool::map | std::views::keys)
		{
			buffer.write(key);
		}
	}

	return buffer;
}

bool SendMempoolMsg::deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	uint32_t mempool_size = 0;
	if (!buffer.read_size(mempool_size))
	{
		*this = std::move(copy);

		return false;
	}
	mempool.clear();
	mempool.reserve(mempool_size);
	for (uint32_t i = 0; i < mempool_size; i++)
	{
		std::string tx;
		if (!buffer.read(tx))
		{
			*this = std::move(copy);

			return false;
		}
		mempool.push_back(tx);
	}

	return true;
}

Opcode SendMempoolMsg::get_opcode() const
{
	return Opcode::SendMempoolMsg;
}
