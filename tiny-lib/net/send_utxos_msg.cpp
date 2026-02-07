#include "net/send_utxos_msg.hpp"

#include "net/msg_cache.hpp"
#include "core/unspent_tx_out.hpp"

void SendUTXOsMsg::handle([[maybe_unused]] const std::shared_ptr<Connection>& con)
{
	MsgCache::set_send_utxos_msg(std::make_shared<SendUTXOsMsg>(*this));
}

BinaryBuffer SendUTXOsMsg::serialize() const
{
	std::vector<std::pair<std::shared_ptr<TxOutPoint>, std::shared_ptr<UTXO>>> utxo_snapshot;
	{
		std::scoped_lock lock(UTXO::mutex);
		utxo_snapshot.reserve(UTXO::map.size());
		for (const auto& [key, value] : UTXO::map)
			utxo_snapshot.emplace_back(key, value);
	}

	BinaryBuffer buffer;
	buffer.write_size(static_cast<uint32_t>(utxo_snapshot.size()));
	for (const auto& [key, value] : utxo_snapshot)
	{
		buffer.write_raw(key->serialize().get_buffer());
		buffer.write_raw(value->serialize().get_buffer());
	}

	return buffer;
}

bool SendUTXOsMsg::deserialize(BinaryBuffer& buffer)
{
	uint32_t utxo_map_size = 0;
	if (!buffer.read_size(utxo_map_size))
		return false;

	std::unordered_map<std::shared_ptr<TxOutPoint>, std::shared_ptr<UTXO>, TxOutPointHash, TxOutPointEqual> new_utxo_map;
	new_utxo_map.reserve(utxo_map_size);
	for (uint32_t i = 0; i < utxo_map_size; i++)
	{
		auto tx_out_point = std::make_shared<TxOutPoint>();
		if (!tx_out_point->deserialize(buffer))
			return false;
		auto utxo = std::make_shared<UTXO>();
		if (!utxo->deserialize(buffer))
			return false;
		new_utxo_map[std::move(tx_out_point)] = std::move(utxo);
	}

	utxo_map = std::move(new_utxo_map);

	return true;
}

Opcode SendUTXOsMsg::get_opcode() const
{
	return Opcode::SendUTXOsMsg;
}
