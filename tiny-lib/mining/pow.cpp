#include "mining/pow.hpp"

#include <cstring>
#include <stdexcept>
#include <limits>
#include <boost/endian/conversion.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "core/chain.hpp"
#include "net/get_block_msg.hpp"
#include "crypto/hash_checker.hpp"
#include "util/log.hpp"
#include "core/mempool.hpp"
#include "mining/merkle_tree.hpp"
#include "mining/mining_backend_factory.hpp"
#include "net/net_client.hpp"
#include "core/net_params.hpp"
#include "crypto/sha256.hpp"
#include "util/utils.hpp"
#include "wallet/wallet.hpp"

std::atomic_bool PoW::mine_interrupt = false;
std::unique_ptr<IMiningBackend> PoW::mining_backend_;

IMiningBackend& PoW::get_backend()
{
	if (!mining_backend_)
		mining_backend_ = MiningBackendFactory::create();
	return *mining_backend_;
}

std::vector<uint8_t> PoW::target_to_bytes(const uint256_t& target)
{
	std::vector<uint8_t> bytes;
	boost::multiprecision::export_bits(target, std::back_inserter(bytes), 8, true);
	if (bytes.size() < 32)
		bytes.insert(bytes.begin(), 32 - bytes.size(), 0);
	return bytes;
}

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
	const auto target_bytes = target_to_bytes(target_hash);

	const auto header_prefix = new_block->header_prefix();
	const auto prefix_bytes = header_prefix.get_buffer();

	auto& backend = get_backend();
	LOG_INFO("Mining with backend: {}", backend.name());

	const auto start = Utils::get_unix_timestamp();
	const auto mine_result = backend.mine(prefix_bytes, target_bytes, mine_interrupt);

	if (mine_interrupt)
	{
		expected = true;
		mine_interrupt.compare_exchange_strong(expected, false);

		LOG_INFO("Mining interrupted");

		return nullptr;
	}

	if (!mine_result.found)
	{
		LOG_ERROR("No nonce satisfies required bits");

		return nullptr;
	}

	new_block->nonce = mine_result.nonce;
	auto duration = Utils::get_unix_timestamp() - start;
	if (duration == 0)
		duration = 1;
	auto khs = mine_result.hash_count / duration / 1000;
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
		else
		{
			const auto mempool_utxo = Mempool::find_utxo_in_mempool(tx_in->to_spend);
			if (mempool_utxo != nullptr)
			{
				spent += mempool_utxo->tx_out->value;
			}
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
