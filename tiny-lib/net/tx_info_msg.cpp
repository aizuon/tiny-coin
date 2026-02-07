#include "net/tx_info_msg.hpp"

#include "util/log.hpp"
#include "core/mempool.hpp"

TxInfoMsg::TxInfoMsg(const std::shared_ptr<::Tx>& tx)
	: tx(tx)
{}

void TxInfoMsg::handle(const std::shared_ptr<Connection>& con)
{
	const auto endpoint = con->socket.remote_endpoint();
	LOG_TRACE("Received transaction {} from peer {}:{}", tx->id(), endpoint.address().to_string(),
		endpoint.port());

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
	auto new_tx = std::make_shared<::Tx>();
	if (!new_tx->deserialize(buffer))
		return false;

	tx = std::move(new_tx);

	return true;
}

Opcode TxInfoMsg::get_opcode() const
{
	return Opcode::TxInfoMsg;
}
