#include "core/chain.hpp"

#include <cassert>
#include <stdexcept>
#include <fstream>
#include <limits>
#include <ranges>
#include <fmt/format.h>

#include "net/block_info_msg.hpp"
#include "util/exceptions.hpp"
#include "crypto/hash_checker.hpp"
#include "util/log.hpp"
#include "core/mempool.hpp"
#include "mining/merkle_tree.hpp"
#include "net/net_client.hpp"
#include "core/net_params.hpp"
#include "mining/pow.hpp"
#include "core/tx_out_point.hpp"
#include "util/uint256_t.hpp"
#include "core/unspent_tx_out.hpp"
#include "util/utils.hpp"

const std::shared_ptr<TxIn> Chain::genesis_tx_in = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>(),
	std::vector<uint8_t>(), -1);
const std::shared_ptr<TxOut> Chain::genesis_tx_out = std::make_shared<TxOut>(
	5000000000, "143UVyz7ooiAv1pMqbwPPpnH4BV9ifJGFF");
const std::shared_ptr<Tx> Chain::genesis_tx = std::make_shared<Tx>(std::vector{ genesis_tx_in },
	std::vector{ genesis_tx_out }, 0);
const std::shared_ptr<Block> Chain::genesis_block = std::make_shared<Block>(
	0, "", "c45c6454c360034ee25d25b0610736cd6ccd10a501c666b3da360c23dffe8535",
	1501821412, 24, 2049638230412955202ULL, std::vector{ genesis_tx });

std::vector<std::shared_ptr<Block>> Chain::active_chain{ genesis_block };
std::vector<std::vector<std::shared_ptr<Block>>> Chain::side_branches{};
std::vector<std::shared_ptr<Block>> Chain::orphan_blocks{};

std::unordered_map<std::string, uint32_t> Chain::active_chain_index{ { genesis_block->id(), 0 } };

std::recursive_mutex Chain::mutex;

std::atomic_bool Chain::initial_block_download_complete = false;

uint32_t Chain::last_saved_height = 0;

uint32_t Chain::get_current_height()
{
	std::scoped_lock lock(mutex);

	return static_cast<uint32_t>(active_chain.size());
}

int64_t Chain::get_median_time_past(uint32_t num_last_blocks)
{
	std::scoped_lock lock(mutex);

	if (num_last_blocks > active_chain.size())
		return 0;

	const uint32_t first_idx = static_cast<uint32_t>(active_chain.size()) - num_last_blocks;

	std::vector<int64_t> timestamps;
	timestamps.reserve(num_last_blocks);
	for (uint32_t i = first_idx; i < active_chain.size(); i++)
		timestamps.push_back(active_chain[i]->timestamp);
	std::ranges::sort(timestamps);

	return timestamps[num_last_blocks / 2];
}

uint32_t Chain::validate_block(const std::shared_ptr<Block>& block)
{
	std::scoped_lock lock(mutex);

	const auto& txs = block->txs;

	if (txs.empty())
		throw BlockValidationException("Transactions empty");

	if (block->timestamp - Utils::get_unix_timestamp() > static_cast<int64_t>(NetParams::MAX_FUTURE_BLOCK_TIME_IN_SECS))
		throw BlockValidationException("Block timestamp too far in future");

	const uint256_t target_hash = uint256_t(1) << (std::numeric_limits<uint8_t>::max() - block->bits);
	if (!HashChecker::is_valid(block->id(), target_hash))
		throw BlockValidationException("Block header does not satisfy bits");

	if (!txs.front()->is_coinbase())
		throw BlockValidationException("First transaction must be coinbase");

	for (size_t i = 1; i < txs.size(); i++)
	{
		if (txs[i]->is_coinbase())
			throw BlockValidationException("No more than one coinbase allowed");
	}

	for (uint32_t i = 0; i < txs.size(); i++)
	{
		try
		{
			txs[i]->validate_basics(i == 0);
		}
		catch (const TxValidationException& ex)
		{
			LOG_ERROR(ex.what());

			LOG_ERROR("Transaction {} in block {} failed validation", txs[i]->id(), block->id());

			throw BlockValidationException(fmt::format("Transaction {} invalid", txs[i]->id()).c_str());
		}
	}

	if (MerkleTree::get_root_of_txs(txs)->value != block->merkle_hash)
		throw BlockValidationException("Merkle hash invalid");

	if (block->timestamp <= get_median_time_past(11))
		throw BlockValidationException("timestamp too old");

	uint32_t prev_block_chain_idx;
	if (block->prev_block_hash.empty())
	{
		prev_block_chain_idx = ACTIVE_CHAIN_IDX;
	}
	else
	{
		auto [prev_block, prev_block_height, prev_block_chain_idx2] = locate_block_in_all_chains(block->prev_block_hash);
		if (prev_block == nullptr)
			throw BlockValidationException(
				fmt::format("Previous block {} not found in any chain", block->prev_block_hash).c_str(), block);

		if (prev_block_chain_idx2 != ACTIVE_CHAIN_IDX)
			return static_cast<uint32_t>(prev_block_chain_idx2);
		if (prev_block->id() != active_chain.back()->id())
			return static_cast<uint32_t>(side_branches.size() + 1);

		prev_block_chain_idx = static_cast<uint32_t>(prev_block_chain_idx2);
	}

	if (PoW::get_next_work_required(block->prev_block_hash) != block->bits)
		throw BlockValidationException("Bits incorrect");

	const int64_t block_height = static_cast<int64_t>(active_chain.size());
	const int64_t block_mtp = get_median_time_past(11);

	Tx::ValidateRequest req;
	req.siblings_in_block.reserve(block->txs.size() - 1);
	req.siblings_in_block.assign(block->txs.begin() + 1, block->txs.end());
	req.allow_utxo_from_mempool = false;
	for (const auto& non_coinbase_tx : req.siblings_in_block)
	{
		try
		{
			non_coinbase_tx->check_lock_time(block_height, block_mtp);
			non_coinbase_tx->validate(req);
		}
		catch (const TxValidationException& ex)
		{
			LOG_ERROR(ex.what());

			const std::string msg = fmt::format("Transaction {} failed to validate", non_coinbase_tx->id());

			LOG_ERROR(msg);

			throw BlockValidationException(msg.c_str());
		}
	}

	return prev_block_chain_idx;
}

int64_t Chain::connect_block(const std::shared_ptr<Block>& block, bool doing_reorg /*= false*/)
{
	std::scoped_lock lock(mutex);

	const auto block_id = block->id();

	std::shared_ptr<Block> located_block;
	if (!doing_reorg)
	{
		auto [located_block2, located_block_height, located_block_chain_idx] = locate_block_in_all_chains(block->id());
		located_block = located_block2;
	}
	else
	{
		auto [located_block2, located_block_height] = locate_block_in_active_chain(block->id());
		located_block = located_block2;
	}
	if (located_block != nullptr)
	{
		LOG_INFO("Ignore already seen block {}", block_id);

		return -1;
	}

	uint32_t chain_idx;
	try
	{
		chain_idx = validate_block(block);
	}
	catch (const BlockValidationException& ex)
	{
		LOG_ERROR(ex.what());

		LOG_ERROR("Block {} failed validation", block_id);
		if (ex.to_orphan != nullptr)
		{
			LOG_INFO("Found orphan block {}", block_id);

			orphan_blocks.push_back(ex.to_orphan);
		}

		return -1;
	}

	if (chain_idx != ACTIVE_CHAIN_IDX && side_branches.size() < chain_idx)
	{
		LOG_INFO("Creating a new side branch with idx {} for block {}", chain_idx, block_id);

		side_branches.emplace_back();
	}

	LOG_INFO("Connecting block {} to chain {}", block_id, chain_idx);

	auto& chain = chain_idx == ACTIVE_CHAIN_IDX ? active_chain : side_branches[chain_idx - 1];
	chain.push_back(block);

	if (chain_idx == ACTIVE_CHAIN_IDX)
	{
		index_block(block, static_cast<uint32_t>(chain.size()) - 1);

		for (const auto& tx : block->txs)
		{
			const auto tx_id = tx->id();

			{
				std::scoped_lock lock_mempool(Mempool::mutex);

				Mempool::map.erase(tx_id);
			}

			if (!tx->is_coinbase())
			{
				for (const auto& tx_in : tx->tx_ins)
				{
					UTXO::remove_from_map(tx_in->to_spend->tx_id, tx_in->to_spend->tx_out_idx);
				}
			}
			for (uint32_t i = 0; i < tx->tx_outs.size(); i++)
			{
				UTXO::add_to_map(tx->tx_outs[i], tx_id, i, tx->is_coinbase(), chain.size());
			}
		}
	}

	if ((!doing_reorg && reorg_if_necessary()) || chain_idx == ACTIVE_CHAIN_IDX)
	{
		PoW::mine_interrupt = true;

		LOG_INFO("Block accepted at height {} with {} txs", active_chain.size() - 1, block->txs.size());
	}

	NetClient::send_msg_random(BlockInfoMsg(block));

	return chain_idx;
}

std::shared_ptr<Block> Chain::disconnect_block(const std::shared_ptr<Block>& block)
{
	std::scoped_lock lock(mutex);

	const auto block_id = block->id();

	auto back = active_chain.back();
	if (block_id != back->id())
		throw std::runtime_error("Block being disconnected must be the tip");

	{
		std::scoped_lock lock_mempool(Mempool::mutex);

		for (const auto& tx : block->txs)
		{
			const auto tx_id = tx->id();

			if (!tx->is_coinbase())
			{
				Mempool::map[tx_id] = tx;
			}

			for (const auto& tx_in : tx->tx_ins)
			{
				if (tx_in->to_spend != nullptr)
				{
					auto [found_tx_out, source_tx, found_tx_out_idx, found_is_coinbase, found_height] = find_tx_out_for_tx_in_in_active_chain(tx_in);

					UTXO::add_to_map(found_tx_out, tx_in->to_spend->tx_id, found_tx_out_idx, found_is_coinbase, found_height);
				}
			}
			for (uint32_t i = 0; i < tx->tx_outs.size(); i++)
			{
				UTXO::remove_from_map(tx_id, i);
			}
		}
	}

	unindex_block(active_chain.back());
	active_chain.pop_back();

	last_saved_height = 0;

	LOG_INFO("Block {} disconnected", block_id);

	return back;
}

std::vector<std::shared_ptr<Block>> Chain::disconnect_to_fork(const std::shared_ptr<Block>& fork_block)
{
	std::scoped_lock lock(mutex);

	std::vector<std::shared_ptr<Block>> disconnected_chain;

	const auto fork_block_id = fork_block->id();
	while (active_chain.back()->id() != fork_block_id)
	{
		disconnected_chain.emplace_back(disconnect_block(active_chain.back()));
	}

	std::ranges::reverse(disconnected_chain);

	return disconnected_chain;
}

bool Chain::reorg_if_necessary()
{
	std::scoped_lock lock(mutex);

	bool reorged = false;

	const auto frozen_side_branches = side_branches;
	uint32_t branch_idx = 1;
	for (const auto& chain : frozen_side_branches)
	{
		auto [fork_block, fork_height] = locate_block_in_active_chain(chain[0]->prev_block_hash);

		uint32_t branch_height = static_cast<uint32_t>(chain.size()) + static_cast<uint32_t>(fork_height);
		if (branch_height > get_current_height())
		{
			LOG_INFO("Attempting reorg of idx {} to active chain, new height of {} vs. {}", branch_idx, branch_height,
				fork_height);

			if (try_reorg(chain, branch_idx, static_cast<uint32_t>(fork_height)))
			{
				reorged = true;
				break;
			}
		}
		branch_idx++;
	}

	return reorged;
}

bool Chain::try_reorg(const std::vector<std::shared_ptr<Block>>& branch, uint32_t branch_idx, uint32_t fork_idx)
{
	std::scoped_lock lock(mutex);

	const auto fork_block = active_chain[fork_idx];

	const auto old_active_chain = disconnect_to_fork(fork_block);

	assert(branch.front()->prev_block_hash == active_chain.back()->id());

	for (const auto& block : branch)
	{
		if (connect_block(block, true) != ACTIVE_CHAIN_IDX)
		{
			rollback_reorg(old_active_chain, fork_block, branch_idx);

			return false;
		}
	}

	side_branches.erase(side_branches.begin() + branch_idx - 1);
	side_branches.push_back(old_active_chain);

	LOG_INFO("Chain reorganized, new height {} with tip {}", active_chain.size(), active_chain.back()->id());

	return true;
}

void Chain::rollback_reorg(const std::vector<std::shared_ptr<Block>>& old_active_chain,
	const std::shared_ptr<Block>& fork_block, uint32_t branch_idx)
{
	std::scoped_lock lock(mutex);

	LOG_ERROR("Reorg of idx {} to active chain failed", branch_idx);

	disconnect_to_fork(fork_block);

	for (const auto& block : old_active_chain)
	{
		const auto connected_block_idx = connect_block(block, true);

		assert(connected_block_idx == ACTIVE_CHAIN_IDX);
	}
}

std::pair<std::shared_ptr<Block>, int64_t> Chain::locate_block_in_chain(const std::string& block_hash,
	const std::vector<std::shared_ptr<Block>>& chain)
{
	std::scoped_lock lock(mutex);

	uint32_t height = 0;
	for (const auto& block : chain)
	{
		if (block->id() == block_hash)
		{
			return { block, height };
		}

		height++;
	}

	return { nullptr, -1 };
}

std::pair<std::shared_ptr<Block>, int64_t> Chain::locate_block_in_active_chain(const std::string& block_hash)
{
	std::scoped_lock lock(mutex);

	const auto it = active_chain_index.find(block_hash);
	if (it != active_chain_index.end())
	{
		return { active_chain[it->second], it->second };
	}

	return { nullptr, -1 };
}

std::tuple<std::shared_ptr<Block>, int64_t, int64_t> Chain::locate_block_in_all_chains(const std::string& block_hash)
{
	std::scoped_lock lock(mutex);

	uint32_t chain_idx = 0;
	auto [located_block, located_block_height] = locate_block_in_active_chain(block_hash);
	if (located_block != nullptr)
		return { located_block, located_block_height, chain_idx };
	chain_idx++;

	for (const auto& side_chain : side_branches)
	{
		auto [located_block, located_block_height] = locate_block_in_chain(block_hash, side_chain);
		if (located_block != nullptr)
			return { located_block, located_block_height, chain_idx };
		chain_idx++;
	}

	return { nullptr, -1, -1 };
}

std::tuple<std::shared_ptr<TxOut>, std::shared_ptr<Tx>, int64_t, bool, int64_t> Chain::find_tx_out_for_tx_in(
	const std::shared_ptr<TxIn>& tx_in, const std::vector<std::shared_ptr<Block>>& chain)
{
	std::scoped_lock lock(mutex);

	for (uint32_t height = 0; height < chain.size(); height++)
	{
		for (const auto& tx : chain[height]->txs)
		{
			const auto& to_spend = tx_in->to_spend;
			if (to_spend->tx_id == tx->id())
			{
				const auto idx = to_spend->tx_out_idx;
				if (idx < 0 || static_cast<size_t>(idx) >= tx->tx_outs.size())
					return { nullptr, nullptr, -1, false, -1 };

				const auto& tx_out = tx->tx_outs[idx];

				return { tx_out, tx, idx, tx->is_coinbase(), static_cast<int64_t>(height) + 1 };
			}
		}
	}

	return { nullptr, nullptr, -1, false, -1 };
}

std::tuple<std::shared_ptr<TxOut>, std::shared_ptr<Tx>, int64_t, bool, int64_t> Chain::find_tx_out_for_tx_in_in_active_chain(
	const std::shared_ptr<TxIn>& tx_in)
{
	return find_tx_out_for_tx_in(tx_in, active_chain);
}

void Chain::save_to_disk()
{
	std::scoped_lock lock(mutex);

	const uint32_t chain_size = static_cast<uint32_t>(active_chain.size() - 1);

	LOG_INFO("Saving chain with {} blocks", active_chain.size());

	if (last_saved_height > 0 && last_saved_height < chain_size)
	{
		std::fstream chain_out(CHAIN_PATH, std::ios::binary | std::ios::in | std::ios::out);
		if (chain_out)
		{
			BinaryBuffer header;
			header.write_size(chain_size);
			auto& header_buf = header.get_buffer();
			chain_out.seekp(0);
			chain_out.write(reinterpret_cast<const char*>(header_buf.data()), header_buf.size());

			chain_out.seekp(0, std::ios::end);
			BinaryBuffer new_data;
			for (uint32_t height = last_saved_height + 1; height < active_chain.size(); height++)
			{
				new_data.write_raw(active_chain[height]->serialize().get_buffer());
			}
			auto& new_buffer = new_data.get_buffer();
			chain_out.write(reinterpret_cast<const char*>(new_buffer.data()), new_buffer.size());
			chain_out.flush();
			chain_out.close();

			const uint32_t prev_saved = last_saved_height;
			last_saved_height = chain_size;

			LOG_INFO("Appended {} new blocks to disk", chain_size - prev_saved);

			return;
		}

		LOG_ERROR("Failed to open {} for appending, falling back to full write", CHAIN_PATH);
	}

	std::ofstream chain_out(CHAIN_PATH, std::ios::binary | std::ios::trunc);
	if (!chain_out)
	{
		LOG_ERROR("Failed to open {} for writing", CHAIN_PATH);
		return;
	}
	BinaryBuffer chain_data;
	chain_data.write_size(chain_size);
	for (uint32_t height = 1; height < active_chain.size(); height++)
	{
		chain_data.write_raw(active_chain[height]->serialize().get_buffer());
	}
	auto& chain_data_buffer = chain_data.get_buffer();
	chain_out.write(reinterpret_cast<const char*>(chain_data_buffer.data()), chain_data_buffer.size());
	chain_out.flush();
	chain_out.close();

	last_saved_height = chain_size;
}

bool Chain::load_from_disk()
{
	std::scoped_lock lock(mutex);

	std::ifstream chain_in(CHAIN_PATH, std::ios::binary);
	if (chain_in.good())
	{
		BinaryBuffer chain_data(std::vector<uint8_t>(std::istreambuf_iterator(chain_in), {}));
		uint32_t block_size = 0;
		if (chain_data.read_size(block_size))
		{
			std::vector<std::shared_ptr<Block>> loaded_chain;
			loaded_chain.reserve(block_size);
			for (uint32_t i = 0; i < block_size; i++)
			{
				auto block = std::make_shared<Block>();
				if (!block->deserialize(chain_data))
				{
					chain_in.close();
					LOG_ERROR("Load chain failed, starting from genesis");

					return false;
				}
				loaded_chain.push_back(block);
			}
			for (const auto& block : loaded_chain)
			{
				if (connect_block(block) != ACTIVE_CHAIN_IDX)
				{
					chain_in.close();
					active_chain.clear();
					active_chain.push_back(genesis_block);
					rebuild_active_chain_index();
					side_branches.clear();
					UTXO::map.clear();
					Mempool::map.clear();

					LOG_ERROR("Load chain failed, starting from genesis");

					return false;
				}
			}
			chain_in.close();

			last_saved_height = static_cast<uint32_t>(active_chain.size() - 1);

			LOG_INFO("Loaded chain with {} blocks", active_chain.size());

			return true;
		}
		LOG_ERROR("Load chain failed, starting from genesis");

		return false;
	}
	LOG_ERROR("Load chain failed, starting from genesis");

	return false;
}

void Chain::reset()
{
	std::scoped_lock lock(mutex);
	active_chain.clear();
	active_chain_index.clear();
	side_branches.clear();
	orphan_blocks.clear();
	Mempool::map.clear();
	UTXO::map.clear();
	last_saved_height = 0;
}

void Chain::index_block(const std::shared_ptr<Block>& block, uint32_t height)
{
	active_chain_index[block->id()] = height;
}

void Chain::unindex_block(const std::shared_ptr<Block>& block)
{
	active_chain_index.erase(block->id());
}

void Chain::rebuild_active_chain_index()
{
	active_chain_index.clear();
	for (uint32_t i = 0; i < active_chain.size(); i++)
	{
		active_chain_index[active_chain[i]->id()] = i;
	}
}
