#include "net/send_mempool_msg.hpp"

#include <ranges>

#include "core/mempool.hpp"
#include "net/msg_cache.hpp"

void SendMempoolMsg::handle([[maybe_unused]] const std::shared_ptr<Connection>& con)
{
	MsgCache::set_send_mempool_msg(std::make_shared<SendMempoolMsg>(*this));
}

BinaryBuffer SendMempoolMsg::serialize() const
{
	std::vector<std::string> keys_snapshot;
	{
		std::scoped_lock lock(Mempool::mutex);
		keys_snapshot.reserve(Mempool::map.size());
		for (const auto& key : Mempool::map | std::views::keys)
			keys_snapshot.push_back(key);
	}

	BinaryBuffer buffer;
	buffer.write_size(static_cast<uint32_t>(keys_snapshot.size()));
	for (const auto& key : keys_snapshot)
	{
		buffer.write(key);
	}

	return buffer;
}

bool SendMempoolMsg::deserialize(BinaryBuffer& buffer)
{
	uint32_t mempool_size = 0;
	if (!buffer.read_size(mempool_size))
		return false;

	std::vector<std::string> new_mempool;
	new_mempool.reserve(mempool_size);
	for (uint32_t i = 0; i < mempool_size; i++)
	{
		std::string tx;
		if (!buffer.read(tx))
			return false;
		new_mempool.push_back(std::move(tx));
	}

	mempool = std::move(new_mempool);

	return true;
}

Opcode SendMempoolMsg::get_opcode() const
{
	return Opcode::SendMempoolMsg;
}
