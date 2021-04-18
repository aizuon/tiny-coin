#include "pch.hpp"

#include "GetBlockMsg.hpp"

GetBlockMsg::GetBlockMsg(const std::string& fromBlockId)
	: FromBlockId(fromBlockId)
{

}

void GetBlockMsg::Handle(const std::shared_ptr<NetClient::Connection>& con) const
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

Opcode GetBlockMsg::GetOpcode() const
{
	return Opcode::GetBlockMsg;
}
