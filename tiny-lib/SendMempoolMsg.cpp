#include "pch.hpp"
#include "SendMempoolMsg.hpp"

#include <ranges>

#include "Mempool.hpp"
#include "MsgCache.hpp"

void SendMempoolMsg::Handle(std::shared_ptr<Connection> con)
{
	MsgCache::SendMempoolMsg = std::make_shared<SendMempoolMsg>(*this);
}

BinaryBuffer SendMempoolMsg::Serialize() const
{
	BinaryBuffer buffer;

	{
		std::scoped_lock lock(Mempool::Mutex);

		buffer.WriteSize(Mempool::Map.size());
		for (const auto& key : Mempool::Map | std::views::keys)
		{
			buffer.Write(key);
		}
	}

	return buffer;
}

bool SendMempoolMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	uint32_t mempool_size = 0;
	if (!buffer.ReadSize(mempool_size))
	{
		*this = std::move(copy);

		return false;
	}
	if (!Mempool.empty())
		Mempool.clear();
	Mempool.reserve(mempool_size);
	for (uint32_t i = 0; i < mempool_size; i++)
	{
		std::string tx;
		if (!buffer.Read(tx))
		{
			*this = std::move(copy);

			return false;
		}
		Mempool.push_back(tx);
	}

	return true;
}

Opcode SendMempoolMsg::GetOpcode() const
{
	return Opcode::SendMempoolMsg;
}
