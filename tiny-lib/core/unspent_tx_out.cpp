#include "core/unspent_tx_out.hpp"

#include <ranges>

#include "util/log.hpp"

UnspentTxOut::UnspentTxOut(std::shared_ptr<::TxOut> tx_out, std::shared_ptr<::TxOutPoint> tx_out_point,
	bool is_coinbase, int64_t height)
	: tx_out(tx_out), tx_out_point(tx_out_point), is_coinbase(is_coinbase), height(height)
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
	auto copy = *this;

	tx_out = std::make_shared<::TxOut>();
	if (!tx_out->deserialize(buffer))
	{
		*this = std::move(copy);

		return false;
	}

	tx_out_point = std::make_shared<::TxOutPoint>();
	if (!tx_out_point->deserialize(buffer))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.read(is_coinbase))
	{
		*this = std::move(copy);

		return false;
	}

	if (!buffer.read(height))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

std::unordered_map<std::shared_ptr<TxOutPoint>, std::shared_ptr<UnspentTxOut>> UnspentTxOut::map;

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

	const auto it = std::ranges::find_if(map,
		[&tx_id, idx](const auto& p)
	{
		const auto& [tx_out_point, utxo] = p;
		return tx_out_point->tx_id == tx_id && tx_out_point->tx_out_idx == idx;
	});
	if (it != map.end())
		map.erase(it);
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

	const auto it = std::ranges::find_if(map,
		[&to_spend](const auto& p)
	{
		const auto& [tx_out_point, utxo] = p;
		return *tx_out_point == *to_spend;
	});
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
			return tx->tx_outs[tx_in->to_spend->tx_out_idx];
		}
	}

	return nullptr;
}

std::shared_ptr<TxOut> UnspentTxOut::find_tx_out_in_map(const std::shared_ptr<TxIn>& tx_in)
{
	std::scoped_lock lock(mutex);

	const auto it = std::ranges::find_if(map,
		[&tx_in](const auto& p)
	{
		const auto& [tx_out_point, utxo] = p;
		return tx_in->to_spend->tx_id == tx_out_point->tx_id && tx_in->to_spend->tx_out_idx == tx_out_point->tx_out_idx;
	});
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
	{
		return true;
	}

	return tied() == obj.tied() && *tx_out == *obj.tx_out && *tx_out_point == *obj.tx_out_point;
}
