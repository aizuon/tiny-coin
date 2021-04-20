#include "pch.hpp"

#include "SendMempoolMsg.hpp"
#include "Mempool.hpp"
#include "MsgCache.hpp"

void SendMempoolMsg::Handle(std::shared_ptr<NetClient::Connection>& con)
{
	MsgCache::SendMempoolMsg = std::make_shared<SendMempoolMsg>(*this);
}

BinaryBuffer SendMempoolMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(Mempool::Map.size());
	for (const auto& [key, value] : Mempool::Map)
	{
		buffer.Write(key);
	}

	return buffer;
}

bool SendMempoolMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	size_t mempoolSize = 0;
	if (!buffer.Read(mempoolSize))
	{
		*this = std::move(copy);

		return false;
	}
	Mempool = std::vector<std::string>();
	Mempool.reserve(mempoolSize);
	for (size_t i = 0; i < mempoolSize; i++)
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
