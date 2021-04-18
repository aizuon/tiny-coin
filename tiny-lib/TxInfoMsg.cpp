#include "pch.hpp"

#include "TxInfoMsg.hpp"
#include "Mempool.hpp"
#include "Log.hpp"

TxInfoMsg::TxInfoMsg(const std::shared_ptr<::Tx>& tx)
	: Tx(tx)
{

}

void TxInfoMsg::Handle(const std::shared_ptr<NetClient::Connection>& con)
{
	LOG_INFO("Recieved transaction {} from peer {}", Tx->Id(), con->Socket.remote_endpoint().address().to_string());

	Mempool::AddTxToMempool(Tx);
}

BinaryBuffer TxInfoMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.WriteRaw(Tx->Serialize().GetBuffer());

	return buffer;
}

bool TxInfoMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	auto tx = std::make_shared<::Tx>();
	if (!tx->Deserialize(buffer))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

Opcode TxInfoMsg::GetOpcode() const
{
	return Opcode::TxInfoMsg;
}
