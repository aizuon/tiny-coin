#include "pch.hpp"

#include "GetBlockMsg.hpp"
#include "BinaryBuffer.hpp"

GetBlockMsg::GetBlockMsg(const std::string& fromBlockId)
	: FromBlockId(fromBlockId)
{

}

void GetBlockMsg::Handle(NetClient::ConnectionHandle con_handle) const
{
	//TODO
}

BinaryBuffer GetBlockMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(FromBlockId);

	return buffer;
}

bool GetBlockMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	if (!buffer.Read(FromBlockId))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}
