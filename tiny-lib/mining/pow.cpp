#include "mining/pow.hpp"

#include <cstring>
#include <stdexcept>
#include <limits>
#include <boost/bind/bind.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/thread/thread.hpp>

#include "core/chain.hpp"
#include "net/get_block_msg.hpp"
#include "crypto/hash_checker.hpp"
#include "util/log.hpp"
#include "core/mempool.hpp"
#include "mining/merkle_tree.hpp"
#include "net/net_client.hpp"
#include "core/net_params.hpp"
#include "crypto/sha256.hpp"
#include "util/utils.hpp"
#include "wallet/wallet.hpp"

std::atomic_bool PoW::mine_interrupt = false;

uint8_t PoW::get_next_work_required(const std::string& prev_block_hash)
{
	if (prev_block_hash.empty())
		return NetParams::INITIAL_DIFFICULTY_BITS;

	auto [prev_block, prev_block_height, prev_block_chain_idx] = Chain::locate_block_in_all_chains(prev_block_hash);
	if ((prev_block_height + 1) % NetParams::DIFFICULTY_PERIOD_IN_BLOCKS != 0)
		return prev_block->bits;

	std::scoped_lock lock(Chain::mutex);
	const auto& period_start_block = Chain::active_chain[std::max(
		prev_block_height - (NetParams::DIFFICULTY_PERIOD_IN_BLOCKS - 1), 0LL)];
	int64_t actual_time_taken = prev_block->timestamp - period_start_block->timestamp;

	constexpr int64_t target_secs = NetParams::DIFFICULTY_PERIOD_IN_SECS_TARGET;
	if (actual_time_taken < target_secs / 4)
		actual_time_taken = target_secs / 4;
	if (actual_time_taken > target_secs * 4)
		actual_time_taken = target_secs * 4;

	const uint256_t old_target = uint256_t(1) << (std::numeric_limits<uint8_t>::max() - prev_block->bits);
	const uint256_t new_target = old_target * actual_time_taken / target_secs;

	const auto new_bits = static_cast<uint8_t>(
		std::numeric_limits<uint8_t>::max() - static_cast<uint8_t>(msb(new_target)));

	return new_bits;
}

uint256_t PoW::get_block_work(uint8_t bits)
{
	const uint256_t target = uint256_t(1) << (std::numeric_limits<uint8_t>::max() - bits);

	return (~target / (target + 1)) + 1;
}

std::shared_ptr<Block> PoW::assemble_and_solve_block(const std::string& pay_coinbase_to_address)
{
	return assemble_and_solve_block(pay_coinbase_to_address, {});
}

std::shared_ptr<Block> PoW::assemble_and_solve_block(const std::string& pay_coinbase_to_address,
	const std::vector<std::shared_ptr<Tx>>& txs)
{
	std::string prev_block_hash;
	{
		std::scoped_lock lock(Chain::mutex);
		prev_block_hash = !Chain::active_chain.empty() ? Chain::active_chain.back()->id() : "";
	}

	auto block = std::make_shared<Block>(0, prev_block_hash, "", Utils::get_unix_timestamp(),
		get_next_work_required(prev_block_hash), 0, txs);

	if (block->txs.empty())
		block = Mempool::select_from_mempool(block);

	const uint64_t fees = calculate_fees(block);
	uint64_t chain_height;
	{
		std::scoped_lock lock(Chain::mutex);
		chain_height = Chain::active_chain.size();
	}
	const auto coinbase_tx = Tx::create_coinbase(pay_coinbase_to_address, get_block_subsidy() + fees,
		chain_height);
	block->txs.insert(block->txs.begin(), coinbase_tx);
	block->merkle_hash = MerkleTree::get_root_of_txs(block->txs)->value;

	if (block->serialize().get_size() > NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES)
		throw std::runtime_error("Transactions specified create a block too large");

	LOG_INFO("Start mining block {} with {} fees", block->id(), fees);

	return mine(block);
}

std::shared_ptr<Block> PoW::mine(const std::shared_ptr<Block>& block)
{
	bool expected = true;
	mine_interrupt.compare_exchange_strong(expected, false);

	auto new_block = std::make_shared<Block>(*block);
	new_block->nonce = 0;
	const uint256_t target_hash = uint256_t(1) << (std::numeric_limits<uint8_t>::max() - new_block->bits);
	int32_t num_threads = static_cast<int32_t>(boost::thread::hardware_concurrency()) - 3;
	if (num_threads <= 0)
		num_threads = 1;
	const uint64_t chunk_size = std::numeric_limits<uint64_t>::max() / num_threads;
	std::atomic_bool found = false;
	std::atomic<uint64_t> found_nonce = 0;
	std::atomic<uint64_t> hash_count = 0;

	const auto header_prefix = new_block->header_prefix();

	const auto start = Utils::get_unix_timestamp();
	boost::thread_group thread_pool;
	for (int32_t i = 0; i < num_threads; i++)
	{
		thread_pool.create_thread(boost::bind(&PoW::mine_chunk, boost::cref(header_prefix), boost::cref(target_hash),
			std::numeric_limits<uint64_t>::min() + chunk_size * i, chunk_size,
			boost::ref(found), boost::ref(found_nonce), boost::ref(hash_count)));
	}
	thread_pool.join_all();

	if (mine_interrupt)
	{
		expected = true;
		mine_interrupt.compare_exchange_strong(expected, false);

		LOG_INFO("Mining interrupted");

		return nullptr;
	}

	if (!found)
	{
		LOG_ERROR("No nonce satisfies required bits");

		return nullptr;
	}

	new_block->nonce = found_nonce;
	auto duration = Utils::get_unix_timestamp() - start;
	if (duration == 0)
		duration = 1;
	auto khs = hash_count / duration / 1000;
	LOG_INFO("Block found => {} s, {} kH/s, {}, {}", duration, khs, new_block->id(), new_block->nonce);

	return new_block;
}

void PoW::mine_forever()
{
	Chain::load_from_disk();

	std::string tip_id;
	{
		std::scoped_lock lock(Chain::mutex);
		if (!Chain::active_chain.empty())
			tip_id = Chain::active_chain.back()->id();
	}

	if (!tip_id.empty())
	{
		constexpr int64_t STALL_TIMEOUT_SECS = 30;
		constexpr uint32_t MAX_RETRIES = 3;
		constexpr int64_t PROGRESS_LOG_INTERVAL_SECS = 10;

		uint32_t retry = 0;
		while (retry < MAX_RETRIES && !Chain::initial_block_download_complete)
		{
			if (!NetClient::send_msg_random(GetBlockMsg(tip_id)))
			{
				LOG_WARN("No peers available for initial block sync, retrying ({}/{})", retry + 1, MAX_RETRIES);
				std::this_thread::sleep_for(std::chrono::seconds(5));
				++retry;
				continue;
			}

			LOG_INFO("Starting initial block sync (attempt {}/{})", retry + 1, MAX_RETRIES);

			uint32_t last_known_height = Chain::get_current_height();
			auto last_progress_time = Utils::get_unix_timestamp();
			auto last_log_time = last_progress_time;

			while (!Chain::initial_block_download_complete)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				const uint32_t current_height = Chain::get_current_height();
				const auto now = Utils::get_unix_timestamp();

				if (current_height > last_known_height)
				{
					last_known_height = current_height;
					last_progress_time = now;
				}

				if (now - last_log_time >= PROGRESS_LOG_INTERVAL_SECS)
				{
					LOG_INFO("Sync in progress, current chain height: {}", current_height);
					last_log_time = now;
				}

				if (now - last_progress_time >= STALL_TIMEOUT_SECS)
				{
					LOG_WARN("Sync stalled at height {} for {} seconds",
						current_height, STALL_TIMEOUT_SECS);
					break;
				}
			}

			if (Chain::initial_block_download_complete)
				break;

			++retry;

			{
				std::scoped_lock lock(Chain::mutex);
				if (!Chain::active_chain.empty())
					tip_id = Chain::active_chain.back()->id();
			}
		}

		if (!Chain::initial_block_download_complete)
		{
			LOG_ERROR("Initial block sync failed after {} retries, resetting chain", MAX_RETRIES);
			Chain::reset();
			Chain::initial_block_download_complete = true;
		}
	}

	const auto [priv_key, pub_key, my_address] = Wallet::init_wallet();
	while (true)
	{
		const auto block = assemble_and_solve_block(my_address);

		if (block != nullptr)
		{
			Chain::connect_block(block);
			Chain::save_to_disk();
		}
	}
}

uint64_t PoW::calculate_fees(const std::shared_ptr<Tx>& tx)
{
	uint64_t spent = 0;
	for (const auto& tx_in : tx->tx_ins)
	{
		const auto utxo = UTXO::find_tx_out_in_map(tx_in);
		if (utxo != nullptr)
		{
			spent += utxo->value;
		}
	}

	uint64_t sent = 0;
	for (const auto& tx_out : tx->tx_outs)
	{
		sent += tx_out->value;
	}

	if (spent < sent)
		return 0;

	return spent - sent;
}

void PoW::mine_chunk(const BinaryBuffer& header_prefix, const uint256_t& target_hash, uint64_t start,
	uint64_t chunk_size, std::atomic_bool& found, std::atomic<uint64_t>& found_nonce,
	std::atomic<uint64_t>& hash_count)
{
	auto prefix_bytes = header_prefix.get_buffer();
	const uint32_t nonce_offset = static_cast<uint32_t>(prefix_bytes.size());
	prefix_bytes.resize(nonce_offset + sizeof(uint64_t));

	uint64_t i = 0;
	uint64_t local_hash_count = 0;
	while (true)
	{
		uint64_t current_nonce = start + i;
		if constexpr (Utils::is_little_endian)
			std::memcpy(prefix_bytes.data() + nonce_offset, &current_nonce, sizeof(uint64_t));
		else
		{
			boost::endian::endian_reverse_inplace(current_nonce);
			std::memcpy(prefix_bytes.data() + nonce_offset, &current_nonce, sizeof(uint64_t));
		}

		const auto hash = SHA256::double_hash_binary(prefix_bytes);
		if (HashChecker::is_valid(hash, target_hash))
		{
			found = true;
			found_nonce = start + i;
			hash_count += local_hash_count;
			return;
		}

		++local_hash_count;
		i++;
		if (i % 4096 == 0)
		{
			hash_count += local_hash_count;
			local_hash_count = 0;
			if (found || mine_interrupt)
				return;
		}
		if (i == chunk_size)
		{
			hash_count += local_hash_count;
			return;
		}
	}
}

uint64_t PoW::calculate_fees(const std::shared_ptr<Block>& block)
{
	uint64_t fee = 0;

	for (const auto& tx : block->txs)
	{
		uint64_t spent = 0;
		for (const auto& tx_in : tx->tx_ins)
		{
			const auto utxo = UTXO::find_tx_out_in_map_or_block(block, tx_in);
			if (utxo != nullptr)
			{
				spent += utxo->value;
			}
		}

		uint64_t sent = 0;
		for (const auto& tx_out : tx->tx_outs)
		{
			sent += tx_out->value;
		}

		if (spent >= sent)
			fee += spent - sent;
	}

	return fee;
}

uint64_t PoW::get_block_subsidy()
{
	uint64_t chain_size;
	{
		std::scoped_lock lock(Chain::mutex);
		chain_size = Chain::active_chain.size();
	}
	const uint32_t halvings = static_cast<uint32_t>(chain_size / NetParams::HALVE_SUBSIDY_AFTER_BLOCKS_NUM);

	if (halvings >= 64)
		return 0;

	return (50 * NetParams::COIN) >> halvings;
}
