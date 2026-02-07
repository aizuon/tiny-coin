#include "core/unspent_tx_out.hpp"

#include <ranges>

#include "util/log.hpp"

UnspentTxOut::UnspentTxOut(std::shared_ptr<::TxOut> tx_out, std::shared_ptr<::TxOutPoint> tx_out_point,
	bool is_coinbase, int64_t height)
	: tx_out(std::move(tx_out)), tx_out_point(std::move(tx_out_point)), is_coinbase(is_coinbase), height(height)
{}

BinaryBuffer UnspentTxOut::serialize() const
{
	BinaryBuffer buffer;

	buffer.write_raw(tx_out->serialize().get_buffer());
	buffer.write_raw(tx_out_point->serialize().get_buffer());
	buffer.write(is_coinbase);
	buffer.write(height);

	return buffer;
}

bool UnspentTxOut::deserialize(BinaryBuffer& buffer)
{
	auto new_tx_out = std::make_shared<::TxOut>();
	if (!new_tx_out->deserialize(buffer))
		return false;

	auto new_tx_out_point = std::make_shared<::TxOutPoint>();
	if (!new_tx_out_point->deserialize(buffer))
		return false;

	bool new_is_coinbase = false;
	if (!buffer.read(new_is_coinbase))
		return false;

	int64_t new_height = -1;
	if (!buffer.read(new_height))
		return false;

	tx_out = std::move(new_tx_out);
	tx_out_point = std::move(new_tx_out_point);
	is_coinbase = new_is_coinbase;
	height = new_height;

	return true;
}

std::unordered_map<std::shared_ptr<TxOutPoint>, std::shared_ptr<UnspentTxOut>, TxOutPointHash, TxOutPointEqual> UnspentTxOut::map;

std::recursive_mutex UnspentTxOut::mutex;

void UnspentTxOut::add_to_map(const std::shared_ptr<::TxOut>& tx_out, const std::string& tx_id, int64_t idx, bool is_coinbase,
	int64_t height)
{
	std::scoped_lock lock(mutex);

	auto tx_out_point = std::make_shared<::TxOutPoint>(tx_id, idx);

	const auto utxo = std::make_shared<UnspentTxOut>(tx_out, tx_out_point, is_coinbase, height);

	LOG_TRACE("Adding TxOutPoint {} to UTXO map", utxo->tx_out_point->tx_id);

	map[utxo->tx_out_point] = utxo;
}

void UnspentTxOut::remove_from_map(const std::string& tx_id, int64_t idx)
{
	std::scoped_lock lock(mutex);

	auto key = std::make_shared<::TxOutPoint>(tx_id, idx);
	map.erase(key);
}

std::shared_ptr<UnspentTxOut> UnspentTxOut::find_in_list(const std::shared_ptr<TxIn>& tx_in,
	const std::vector<std::shared_ptr<Tx>>& txs)
{
	for (const auto& tx : txs)
	{
		const auto& to_spend = tx_in->to_spend;

		if (tx->id() == to_spend->tx_id)
		{
			if (to_spend->tx_out_idx < 0 || static_cast<size_t>(to_spend->tx_out_idx) >= tx->tx_outs.size())
				return nullptr;

			auto& matching_tx_out = tx->tx_outs[to_spend->tx_out_idx];
			auto tx_out_point = std::make_shared<::TxOutPoint>(to_spend->tx_id, to_spend->tx_out_idx);
			return std::make_shared<UnspentTxOut>(matching_tx_out, tx_out_point, false, -1);
		}
	}

	return nullptr;
}

std::shared_ptr<UnspentTxOut> UnspentTxOut::find_in_map(const std::shared_ptr<::TxOutPoint>& to_spend)
{
	std::scoped_lock lock(mutex);

	const auto it = map.find(to_spend);
	if (it != map.end())
		return it->second;

	return nullptr;
}

std::shared_ptr<TxOut> UnspentTxOut::find_tx_out_in_block(const std::shared_ptr<Block>& block,
	const std::shared_ptr<TxIn>& tx_in)
{
	for (const auto& tx : block->txs)
	{
		if (tx->id() == tx_in->to_spend->tx_id)
		{
			const auto idx = tx_in->to_spend->tx_out_idx;
			if (idx < 0 || static_cast<size_t>(idx) >= tx->tx_outs.size())
				return nullptr;
			return tx->tx_outs[idx];
		}
	}

	return nullptr;
}

std::shared_ptr<TxOut> UnspentTxOut::find_tx_out_in_map(const std::shared_ptr<TxIn>& tx_in)
{
	std::scoped_lock lock(mutex);

	const auto it = map.find(tx_in->to_spend);
	if (it != map.end())
		return it->second->tx_out;

	return nullptr;
}

std::shared_ptr<TxOut> UnspentTxOut::find_tx_out_in_map_or_block(const std::shared_ptr<Block>& block,
	const std::shared_ptr<TxIn>& tx_in)
{
	auto utxo = find_tx_out_in_map(tx_in);
	if (utxo != nullptr)
		return utxo;

	return find_tx_out_in_block(block, tx_in);
}

bool UnspentTxOut::operator==(const UnspentTxOut& obj) const
{
	if (this == &obj)
		return true;

	if (tied() != obj.tied())
		return false;

	if (tx_out == nullptr || obj.tx_out == nullptr || tx_out_point == nullptr || obj.tx_out_point == nullptr)
		return tx_out == obj.tx_out && tx_out_point == obj.tx_out_point;

	return *tx_out == *obj.tx_out && *tx_out_point == *obj.tx_out_point;
}
