#include "net/send_utxos_msg.hpp"

#include "net/msg_cache.hpp"
#include "core/unspent_tx_out.hpp"

void SendUTXOsMsg::handle(const std::shared_ptr<Connection>& con)
{
	MsgCache::send_utxos_msg = std::make_shared<SendUTXOsMsg>(*this);
}

BinaryBuffer SendUTXOsMsg::serialize() const
{
	BinaryBuffer buffer;

	{
		std::scoped_lock lock(UTXO::mutex);

		buffer.write_size(static_cast<uint32_t>(UTXO::map.size()));
		for (const auto& [key, value] : UTXO::map)
		{
			buffer.write_raw(key->serialize().get_buffer());
			buffer.write_raw(value->serialize().get_buffer());
		}
	}

	return buffer;
}

bool SendUTXOsMsg::deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	uint32_t utxo_map_size = 0;
	if (!buffer.read_size(utxo_map_size))
	{
		*this = std::move(copy);

		return false;
	}
	utxo_map.clear();
	utxo_map.reserve(utxo_map_size);
	for (uint32_t i = 0; i < utxo_map_size; i++)
	{
		auto tx_out_point = std::make_shared<TxOutPoint>();
		if (!tx_out_point->deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		const auto utxo = std::make_shared<UTXO>();
		if (!utxo->deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		utxo_map[tx_out_point] = utxo;
	}

	return true;
}

Opcode SendUTXOsMsg::get_opcode() const
{
	return Opcode::SendUTXOsMsg;
}
