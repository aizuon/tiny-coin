#include "core/mempool.hpp"

#include <ranges>
#include <utility>

#include "util/binary_buffer.hpp"
#include "util/exceptions.hpp"
#include "util/log.hpp"
#include "net/net_client.hpp"
#include "core/net_params.hpp"
#include "mining/pow.hpp"
#include "net/tx_info_msg.hpp"

std::unordered_map<std::string, std::shared_ptr<Tx>> Mempool::map;

std::vector<std::shared_ptr<Tx>> Mempool::orphaned_txs;

std::recursive_mutex Mempool::mutex;

std::shared_ptr<UTXO> Mempool::find_utxo_in_mempool(const std::shared_ptr<TxOutPoint>& tx_out_point)
{
	std::scoped_lock lock(mutex);

	const auto it = map.find(tx_out_point->tx_id);
	if (it == map.end())
		return nullptr;

	const auto& tx = it->second;
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

	std::vector<std::pair<std::string, std::shared_ptr<Tx>>> map_vector(map.begin(), map.end());

	std::unordered_map<std::string, uint64_t> fee_cache;
	fee_cache.reserve(map_vector.size());
	for (const auto& [tx_id, tx] : map_vector)
		fee_cache[tx_id] = PoW::calculate_fees(tx);

	std::ranges::sort(map_vector,
		[&fee_cache](const std::pair<std::string, std::shared_ptr<Tx>>& a,
			const std::pair<std::string, std::shared_ptr<Tx>>& b) -> bool
	{
		return fee_cache[a.first] > fee_cache[b.first];
	});

	std::set<std::string> added_to_block;
	uint32_t current_block_size = new_block->serialize().get_size();
	for (const auto& tx_id : map_vector | std::views::keys)
	{
		new_block = try_add_to_block(new_block, tx_id, added_to_block, current_block_size);
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

	map[tx_id] = tx;

	LOG_TRACE("Transaction {} added to mempool", tx_id);

	NetClient::send_msg_random(TxInfoMsg(tx));
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

	const auto& tx = tx_it->second;

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
