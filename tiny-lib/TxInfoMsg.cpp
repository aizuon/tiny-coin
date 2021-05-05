#include "pch.hpp"
#include "TxInfoMsg.hpp"

#include "Log.hpp"
#include "Mempool.hpp"

TxInfoMsg::TxInfoMsg(const std::shared_ptr<::Tx>& tx)
	: Tx(tx)
{
}

void TxInfoMsg::Handle(std::shared_ptr<Connection>& con)
{
	LOG_INFO("Recieved transaction {} from peer {}:{}", Tx->Id(), con->Socket.remote_endpoint().address().to_string(),
	         con->Socket.remote_endpoint().port());

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

	Tx = std::make_shared<::Tx>();
	if (!Tx->Deserialize(buffer))
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
