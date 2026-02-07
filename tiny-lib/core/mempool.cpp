#include "core/mempool.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

#include "util/binary_buffer.hpp"
#include "util/exceptions.hpp"
#include "util/log.hpp"
#include "net/net_client.hpp"
#include "core/net_params.hpp"
#include "mining/pow.hpp"
#include "net/tx_info_msg.hpp"

std::unordered_map<std::string, Mempool::MempoolEntry> Mempool::map;

std::vector<std::shared_ptr<Tx>> Mempool::orphaned_txs;

std::recursive_mutex Mempool::mutex;

uint64_t Mempool::total_size_bytes = 0;

std::shared_ptr<UTXO> Mempool::find_utxo_in_mempool(const std::shared_ptr<TxOutPoint>& tx_out_point)
{
	std::scoped_lock lock(mutex);

	const auto it = map.find(tx_out_point->tx_id);
	if (it == map.end())
		return nullptr;

	const auto& tx = it->second.tx;
	if (tx_out_point->tx_out_idx < 0 || static_cast<size_t>(tx_out_point->tx_out_idx) >= tx->tx_outs.size())
	{
		LOG_ERROR("Unable to find UTXO in mempool for {}", tx_out_point->tx_id);

		return nullptr;
	}

	const auto& tx_out = tx->tx_outs[tx_out_point->tx_out_idx];

	return std::make_shared<UTXO>(tx_out, tx_out_point, false, -1);
}

std::shared_ptr<Block> Mempool::select_from_mempool(const std::shared_ptr<Block>& block)
{
	std::scoped_lock lock(mutex);

	auto new_block = std::make_shared<Block>(*block);

	std::set<std::string> added_to_block;
	uint32_t current_block_size = new_block->serialize().get_size();

	while (true)
	{
		std::string best_tx_id;
		uint64_t best_effective_rate = 0;

		for (const auto& [tx_id, entry] : map)
		{
			if (added_to_block.contains(tx_id))
				continue;

			const uint64_t pkg_rate = compute_ancestor_package_fee_rate(tx_id, added_to_block);
			const uint64_t effective_rate = std::max(entry.fee_rate, pkg_rate);

			if (effective_rate > best_effective_rate || best_tx_id.empty())
			{
				best_effective_rate = effective_rate;
				best_tx_id = tx_id;
			}
		}

		if (best_tx_id.empty())
			break;

		new_block = try_add_to_block(new_block, best_tx_id, added_to_block, current_block_size);
		if (new_block == nullptr)
		{
			LOG_ERROR("Block assembly failed, returning partial block");

			return std::make_shared<Block>(*block);
		}
	}

	return new_block;
}

void Mempool::add_tx_to_mempool(const std::shared_ptr<Tx>& tx)
{
	std::scoped_lock lock(mutex);

	const auto tx_id = tx->id();
	if (map.contains(tx_id))
	{
		LOG_INFO("Transaction {} already seen", tx_id);

		return;
	}

	if (has_dust_outputs(tx))
	{
		LOG_ERROR("Transaction {} rejected: contains dust output(s) below threshold {}",
			tx_id, NetParams::DUST_THRESHOLD);

		return;
	}

	try
	{
		tx->validate(Tx::ValidateRequest());
	}
	catch (const TxValidationException& ex)
	{
		LOG_ERROR(ex.what());

		if (ex.to_orphan != nullptr)
		{
			LOG_INFO("Transaction {} submitted as orphan", ex.to_orphan->id());

			orphaned_txs.push_back(ex.to_orphan);

			return;
		}
		LOG_ERROR("Transaction {} rejected", tx_id);

		return;
	}

	if (violates_chain_limits(tx))
	{
		LOG_ERROR("Transaction {} rejected: exceeds ancestor/descendant chain limit", tx_id);

		return;
	}

	const auto conflicting = find_conflicting_txs(tx);
	if (!conflicting.empty())
	{
		if (!try_replace_by_fee(tx))
			return;

		NetClient::send_msg_random(TxInfoMsg(tx));

		return;
	}

	MempoolEntry entry;
	entry.tx = tx;
	entry.serialized_size = tx->serialize().get_size();
	entry.fee = PoW::calculate_fees(tx);
	entry.fee_rate = entry.serialized_size > 0 ? entry.fee / entry.serialized_size : 0;
	entry.insertion_time = std::chrono::steady_clock::now();

	total_size_bytes += entry.serialized_size;
	map[tx_id] = std::move(entry);

	LOG_TRACE("Transaction {} added to mempool (size now {} bytes)", tx_id, total_size_bytes);

	enforce_size_cap();

	NetClient::send_msg_random(TxInfoMsg(tx));
}

bool Mempool::try_replace_by_fee(const std::shared_ptr<Tx>& tx)
{
	const auto tx_id = tx->id();
	const auto conflicting = find_conflicting_txs(tx);

	for (const auto& existing_tx : conflicting)
	{
		if (!existing_tx->signals_rbf())
		{
			LOG_ERROR("Transaction {} conflicts with non-replaceable transaction {}", tx_id, existing_tx->id());

			return false;
		}
	}

	uint64_t conflicting_fees = 0;
	std::set<std::string> to_remove;
	for (const auto& existing_tx : conflicting)
	{
		const auto existing_id = existing_tx->id();
		const auto it = map.find(existing_id);
		if (it != map.end())
			conflicting_fees += it->second.fee;
		to_remove.insert(existing_id);

		for (const auto& desc_id : find_descendant_tx_ids(existing_id))
		{
			const auto desc_it = map.find(desc_id);
			if (desc_it != map.end())
			{
				conflicting_fees += desc_it->second.fee;
				to_remove.insert(desc_id);
			}
		}
	}

	const uint64_t new_fee = PoW::calculate_fees(tx);
	if (new_fee <= conflicting_fees)
	{
		LOG_ERROR("Replacement transaction {} fee ({}) does not exceed conflicting fees ({})",
			tx_id, new_fee, conflicting_fees);

		return false;
	}

	const uint64_t tx_size = tx->serialize().get_size();
	const uint64_t min_increment = (tx_size * NetParams::INCREMENTAL_RELAY_FEE + 999) / 1000;
	if (new_fee < conflicting_fees + min_increment)
	{
		LOG_ERROR("Replacement transaction {} fee ({}) does not meet incremental relay fee requirement ({} + {} = {})",
			tx_id, new_fee, conflicting_fees, min_increment, conflicting_fees + min_increment);

		return false;
	}

	for (const auto& id : to_remove)
	{
		LOG_TRACE("RBF: removing conflicting transaction {}", id);
		remove_entry(id);
	}

	MempoolEntry entry;
	entry.tx = tx;
	entry.serialized_size = static_cast<uint32_t>(tx_size);
	entry.fee = new_fee;
	entry.fee_rate = tx_size > 0 ? new_fee / tx_size : 0;
	entry.insertion_time = std::chrono::steady_clock::now();

	total_size_bytes += entry.serialized_size;
	map[tx_id] = std::move(entry);

	LOG_INFO("RBF: transaction {} replaced {} conflicting transaction(s) (fee {} > {})",
		tx_id, to_remove.size(), new_fee, conflicting_fees);

	enforce_size_cap();

	return true;
}

std::vector<std::shared_ptr<Tx>> Mempool::find_conflicting_txs(const std::shared_ptr<Tx>& tx)
{
	std::vector<std::shared_ptr<Tx>> conflicts;

	for (const auto& tx_in : tx->tx_ins)
	{
		if (tx_in->to_spend == nullptr)
			continue;

		for (const auto& [existing_id, existing_entry] : map)
		{
			for (const auto& existing_in : existing_entry.tx->tx_ins)
			{
				if (existing_in->to_spend == nullptr)
					continue;

				if (*existing_in->to_spend == *tx_in->to_spend)
				{
					bool already_found = false;
					for (const auto& c : conflicts)
					{
						if (c->id() == existing_id)
						{
							already_found = true;
							break;
						}
					}
					if (!already_found)
						conflicts.push_back(existing_entry.tx);
				}
			}
		}
	}

	return conflicts;
}

std::vector<std::string> Mempool::find_descendant_tx_ids(const std::string& tx_id)
{
	std::vector<std::string> descendants;
	std::vector<std::string> queue{ tx_id };

	while (!queue.empty())
	{
		const auto current = queue.back();
		queue.pop_back();

		for (const auto& [id, entry] : map)
		{
			if (id == current)
				continue;

			for (const auto& tx_in : entry.tx->tx_ins)
			{
				if (tx_in->to_spend != nullptr && tx_in->to_spend->tx_id == current)
				{
					bool already_found = false;
					for (const auto& d : descendants)
					{
						if (d == id)
						{
							already_found = true;
							break;
						}
					}
					if (!already_found)
					{
						descendants.push_back(id);
						queue.push_back(id);
					}
					break;
				}
			}
		}
	}

	return descendants;
}

bool Mempool::check_block_size(uint32_t current_size)
{
	return current_size < NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES;
}

std::shared_ptr<Block> Mempool::try_add_to_block(std::shared_ptr<Block> block, const std::string& tx_id,
	std::set<std::string>& added_to_block, uint32_t& current_block_size)

{
	std::scoped_lock lock(mutex);

	if (added_to_block.contains(tx_id))
		return block;

	const auto tx_it = map.find(tx_id);
	if (tx_it == map.end())
		return block;

	const auto& tx = tx_it->second.tx;

	for (const auto& tx_in : tx->tx_ins)
	{
		const auto& to_spend = tx_in->to_spend;

		if (UTXO::find_in_map(to_spend) != nullptr)
			continue;

		const auto& in_mempool = find_utxo_in_mempool(to_spend);
		if (in_mempool == nullptr)
		{
			LOG_ERROR("Unable to find UTXO for {}", tx_in->to_spend->tx_id);

			return nullptr;
		}

		block = try_add_to_block(block, in_mempool->tx_out_point->tx_id, added_to_block, current_block_size);
		if (block == nullptr)
		{
			LOG_ERROR("Unable to add parent");

			return nullptr;
		}
	}

	const uint32_t tx_serialized_size = tx->serialize().get_size();
	if (!check_block_size(current_block_size + tx_serialized_size))
		return block;

	block->txs.push_back(tx);
	current_block_size += tx_serialized_size;

	LOG_TRACE("Added transaction {} to block {}", tx_id, block->id());

	added_to_block.insert(tx_id);

	return block;
}

std::vector<std::string> Mempool::find_ancestor_tx_ids(const std::string& tx_id)
{
	std::vector<std::string> ancestors;
	std::vector<std::string> queue{ tx_id };

	while (!queue.empty())
	{
		const auto current = queue.back();
		queue.pop_back();

		const auto it = map.find(current);
		if (it == map.end())
			continue;

		for (const auto& tx_in : it->second.tx->tx_ins)
		{
			if (tx_in->to_spend == nullptr)
				continue;

			const auto& parent_id = tx_in->to_spend->tx_id;
			if (!map.contains(parent_id))
				continue;

			bool already_found = false;
			for (const auto& a : ancestors)
			{
				if (a == parent_id)
				{
					already_found = true;
					break;
				}
			}
			if (!already_found)
			{
				ancestors.push_back(parent_id);
				queue.push_back(parent_id);
			}
		}
	}

	return ancestors;
}

uint64_t Mempool::compute_ancestor_package_fee_rate(const std::string& tx_id,
	const std::set<std::string>& excluded)
{
	const auto it = map.find(tx_id);
	if (it == map.end())
		return 0;

	uint64_t total_fee = it->second.fee;
	uint64_t total_size = it->second.serialized_size;

	const auto ancestors = find_ancestor_tx_ids(tx_id);
	for (const auto& anc_id : ancestors)
	{
		if (excluded.contains(anc_id))
			continue;

		const auto anc_it = map.find(anc_id);
		if (anc_it == map.end())
			continue;

		total_fee += anc_it->second.fee;
		total_size += anc_it->second.serialized_size;
	}

	return total_size > 0 ? total_fee / total_size : 0;
}

bool Mempool::violates_chain_limits(const std::shared_ptr<Tx>& tx)
{
	uint32_t ancestor_count = 0;
	for (const auto& tx_in : tx->tx_ins)
	{
		if (tx_in->to_spend == nullptr)
			continue;

		const auto& parent_id = tx_in->to_spend->tx_id;
		if (!map.contains(parent_id))
			continue;

		const auto parent_ancestors = find_ancestor_tx_ids(parent_id);
		const uint32_t depth = static_cast<uint32_t>(parent_ancestors.size()) + 1;
		if (depth > ancestor_count)
			ancestor_count = depth;
	}

	if (ancestor_count >= NetParams::MAX_ANCESTOR_COUNT)
		return true;

	for (const auto& tx_in : tx->tx_ins)
	{
		if (tx_in->to_spend == nullptr)
			continue;

		const auto& parent_id = tx_in->to_spend->tx_id;
		if (!map.contains(parent_id))
			continue;

		auto ancestors = find_ancestor_tx_ids(parent_id);
		ancestors.push_back(parent_id);

		for (const auto& anc_id : ancestors)
		{
			const auto descendants = find_descendant_tx_ids(anc_id);
			if (static_cast<uint32_t>(descendants.size()) + 1 >= NetParams::MAX_DESCENDANT_COUNT)
				return true;
		}
	}

	return false;
}

bool Mempool::has_dust_outputs(const std::shared_ptr<Tx>& tx)
{
	if (tx->is_coinbase())
		return false;

	for (const auto& tx_out : tx->tx_outs)
	{
		if (tx_out->value < NetParams::DUST_THRESHOLD)
			return true;
	}

	return false;
}

void Mempool::enforce_size_cap()
{
	while (total_size_bytes > NetParams::MAX_MEMPOOL_SIZE_BYTES && !map.empty())
	{
		std::string worst_id;
		uint64_t worst_fee_rate = UINT64_MAX;

		for (const auto& [id, entry] : map)
		{
			if (entry.fee_rate < worst_fee_rate)
			{
				worst_fee_rate = entry.fee_rate;
				worst_id = id;
			}
		}

		if (worst_id.empty())
			break;

		auto desc_ids = find_descendant_tx_ids(worst_id);
		desc_ids.push_back(worst_id);

		for (const auto& id : desc_ids)
		{
			LOG_TRACE("Evicting transaction {} (fee rate {}) to enforce mempool size cap", id,
				map.contains(id) ? map[id].fee_rate : 0);
			remove_entry(id);
		}
	}
}

void Mempool::remove_entry(const std::string& tx_id)
{
	const auto it = map.find(tx_id);
	if (it == map.end())
		return;

	if (total_size_bytes >= it->second.serialized_size)
		total_size_bytes -= it->second.serialized_size;
	else
		total_size_bytes = 0;

	map.erase(it);
}

void Mempool::expire_old_transactions()
{
	std::scoped_lock lock(mutex);

	const auto now = std::chrono::steady_clock::now();
	std::vector<std::string> to_remove;

	for (const auto& [id, entry] : map)
	{
		const auto age = std::chrono::duration_cast<std::chrono::seconds>(now - entry.insertion_time).count();
		if (age >= NetParams::MEMPOOL_TX_EXPIRE_SECS)
			to_remove.push_back(id);
	}

	for (const auto& id : to_remove)
	{
		LOG_INFO("Expiring transaction {} from mempool (exceeded TTL of {} seconds)", id,
			NetParams::MEMPOOL_TX_EXPIRE_SECS);

		auto desc_ids = find_descendant_tx_ids(id);
		remove_entry(id);
		for (const auto& desc_id : desc_ids)
			remove_entry(desc_id);
	}
}
