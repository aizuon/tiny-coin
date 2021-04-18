#include "pch.hpp"

#include "GetMempoolMsg.hpp"

GetMempoolMsg::GetMempoolMsg(const std::vector<std::string>& mempool)
	: Mempool(mempool)
{

}

void GetMempoolMsg::Handle(NetClient::ConnectionHandle con_handle) const
{
	//TODO
}

BinaryBuffer GetMempoolMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(Mempool.size());
	for (const auto& tx : Mempool)
	{
		buffer.Write(tx);
	}

	return buffer;
}

bool GetMempoolMsg::Deserialize(BinaryBuffer& buffer)
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

Opcode GetMempoolMsg::GetOpcode() const
{
	return Opcode::GetMempoolMsg;
}
