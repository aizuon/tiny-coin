#include "net/tx_info_msg.hpp"

#include "util/log.hpp"
#include "core/mempool.hpp"

TxInfoMsg::TxInfoMsg(const std::shared_ptr<::Tx>& tx)
	: tx(tx)
{}

void TxInfoMsg::handle(const std::shared_ptr<Connection>& con)
{
	LOG_TRACE("Recieved transaction {} from peer {}:{}", tx->id(), con->socket.remote_endpoint().address().to_string(),
		con->socket.remote_endpoint().port());

	Mempool::add_tx_to_mempool(tx);
}

BinaryBuffer TxInfoMsg::serialize() const
{
	BinaryBuffer buffer;

	buffer.write_raw(tx->serialize().get_buffer());

	return buffer;
}

bool TxInfoMsg::deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	tx = std::make_shared<::Tx>();
	if (!tx->deserialize(buffer))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

Opcode TxInfoMsg::get_opcode() const
{
	return Opcode::TxInfoMsg;
}
