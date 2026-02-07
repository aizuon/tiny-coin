#include "wallet/wallet.hpp"

#include <chrono>
#include <fstream>
#include <iterator>
#include <limits>
#include <mutex>
#include <ranges>
#include <thread>


#include "crypto/base58.hpp"
#include "core/chain.hpp"
#include "crypto/ecdsa.hpp"
#include "net/get_active_chain_msg.hpp"
#include "net/get_mempool_msg.hpp"
#include "net/get_utxos_msg.hpp"
#include "util/log.hpp"
#include "core/mempool.hpp"
#include "net/msg_cache.hpp"
#include "net/msg_serializer.hpp"
#include "net/net_client.hpp"
#include "crypto/ripemd160.hpp"
#include "net/send_active_chain_msg.hpp"
#include "net/send_mempool_msg.hpp"
#include "net/send_utxos_msg.hpp"
#include "crypto/sha256.hpp"
#include "core/tx.hpp"
#include "net/tx_info_msg.hpp"
#include "core/unspent_tx_out.hpp"
#include "util/utils.hpp"
#include "wallet/hd_wallet.hpp"

std::string Wallet::wallet_path = DEFAULT_WALLET_PATH;
std::string Wallet::hd_wallet_path = DEFAULT_HD_WALLET_PATH;

std::string Wallet::pub_key_to_address(const std::vector<uint8_t>& pub_key)
{
	const auto sha256 = SHA256::hash_binary(pub_key);

	const auto ripe = RIPEMD160::hash_binary(sha256);

	std::vector<uint8_t> versioned_ripe;
	versioned_ripe.reserve(1 + ripe.size() + 4);
	versioned_ripe.push_back(0x00);
	versioned_ripe.insert(versioned_ripe.end(), ripe.begin(), ripe.end());

	const auto sha256d = SHA256::double_hash_binary(versioned_ripe);

	versioned_ripe.insert(versioned_ripe.end(), sha256d.begin(), sha256d.begin() + 4);

	return Base58::encode(versioned_ripe);
}

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> Wallet::get_wallet(const std::string& wallet_path)
{
	std::vector<uint8_t> priv_key;
	std::vector<uint8_t> pub_key;
	std::string address;

	std::ifstream wallet_in(wallet_path, std::ios::binary);
	if (wallet_in.good())
	{
		priv_key = std::vector<uint8_t>(std::istreambuf_iterator(wallet_in), {});
		pub_key = ECDSA::get_pub_key_from_priv_key(priv_key);
		address = pub_key_to_address(pub_key);
	}
	else
	{
		LOG_INFO("Generating new wallet {}", wallet_path);

		auto [priv_key2, pub_key2] = ECDSA::generate();
		priv_key = std::move(priv_key2);
		pub_key = std::move(pub_key2);
		address = pub_key_to_address(pub_key);

		std::ofstream wallet_out(wallet_path, std::ios::binary);
		wallet_out.write(reinterpret_cast<const char*>(priv_key.data()), priv_key.size());
		wallet_out.flush();
		wallet_out.close();
	}

	return { priv_key, pub_key, address };
}

void Wallet::print_wallet_address(const std::string& wallet_path)
{
	const auto [priv_key, pub_key, address] = get_wallet(wallet_path);

	LOG_INFO("Wallet {} belongs to address {}", wallet_path, address);
}

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> Wallet::init_wallet(const std::string& wallet_path)
{
	Wallet::wallet_path = wallet_path;

	const auto [priv_key, pub_key, address] = get_wallet(wallet_path);

	static std::once_flag print_flag;
	std::call_once(print_flag, [&address]()
	{
		LOG_INFO("Your address is {}", address);
	});

	return { priv_key, pub_key, address };
}

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> Wallet::init_wallet()
{
	return init_wallet(wallet_path);
}

std::shared_ptr<TxIn> Wallet::build_tx_in(const std::vector<uint8_t>& priv_key,
	const std::vector<uint8_t>& pub_key,
	const std::shared_ptr<TxOutPoint>& tx_out_point,
	const std::vector<std::shared_ptr<TxOut>>& tx_outs)
{
	return build_tx_in(priv_key, pub_key, tx_out_point, tx_outs, TxIn::SEQUENCE_RBF);
}

std::shared_ptr<TxIn> Wallet::build_tx_in(const std::vector<uint8_t>& priv_key,
	const std::vector<uint8_t>& pub_key,
	const std::shared_ptr<TxOutPoint>& tx_out_point,
	const std::vector<std::shared_ptr<TxOut>>& tx_outs,
	int32_t sequence)
{
	const auto spend_msg = MsgSerializer::build_spend_msg(tx_out_point, pub_key, sequence, tx_outs);
	auto unlock_sig = ECDSA::sign_msg(spend_msg, priv_key);

	return std::make_shared<TxIn>(tx_out_point, unlock_sig, pub_key, sequence);
}

std::shared_ptr<Tx> Wallet::send_value_miner(uint64_t value, uint64_t fee, const std::string& address,
	const std::vector<uint8_t>& priv_key, int64_t lock_time)
{
	auto tx = build_tx_miner(value, fee, address, priv_key, lock_time);
	if (tx == nullptr)
		return nullptr;
	LOG_INFO("Built transaction {}, adding to mempool", tx->id());
	Mempool::add_tx_to_mempool(tx);
	NetClient::send_msg_random(TxInfoMsg(tx));

	return tx;
}

std::shared_ptr<Tx> Wallet::send_value(uint64_t value, uint64_t fee, const std::string& address,
	const std::vector<uint8_t>& priv_key, int64_t lock_time)
{
	auto tx = build_tx(value, fee, address, priv_key, lock_time);
	if (tx == nullptr)
		return nullptr;
	LOG_INFO("Built transaction {}, broadcasting", tx->id());
	if (!NetClient::send_msg_random(TxInfoMsg(tx)))
	{
		LOG_ERROR("No connection to send transaction");
	}

	return tx;
}

std::shared_ptr<Tx> Wallet::rbf_tx_miner(const std::string& tx_id, uint64_t new_fee_per_byte,
	const std::vector<uint8_t>& priv_key)
{
	std::shared_ptr<Tx> original_tx;
	{
		std::scoped_lock lock(Mempool::mutex);
		const auto it = Mempool::map.find(tx_id);
		if (it == Mempool::map.end())
		{
			LOG_ERROR("Transaction {} not found in mempool", tx_id);
			return nullptr;
		}
		original_tx = it->second.tx;
	}

	if (!original_tx->signals_rbf())
	{
		LOG_ERROR("Transaction {} does not signal RBF", tx_id);
		return nullptr;
	}

	const auto pub_key = ECDSA::get_pub_key_from_priv_key(priv_key);
	auto replacement = build_rbf_replacement(original_tx, new_fee_per_byte, priv_key, pub_key);
	if (replacement == nullptr)
		return nullptr;

	LOG_INFO("Built RBF replacement {}, adding to mempool", replacement->id());
	Mempool::add_tx_to_mempool(replacement);
	NetClient::send_msg_random(TxInfoMsg(replacement));

	return replacement;
}

std::shared_ptr<Tx> Wallet::rbf_tx(const std::string& tx_id, uint64_t new_fee_per_byte,
	const std::vector<uint8_t>& priv_key)
{
	MsgCache::set_send_mempool_msg(nullptr);

	if (!NetClient::send_msg_random(GetMempoolMsg()))
	{
		LOG_ERROR("No connection to query mempool");
		return nullptr;
	}

	const auto start = Utils::get_unix_timestamp();
	std::shared_ptr<SendMempoolMsg> cached_mempool_msg;
	while (true)
	{
		cached_mempool_msg = MsgCache::get_send_mempool_msg();
		if (cached_mempool_msg != nullptr)
			break;
		if (Utils::get_unix_timestamp() - start > MsgCache::MAX_MSG_AWAIT_TIME_IN_SECS)
		{
			LOG_ERROR("Timeout on GetMempoolMsg");
			return nullptr;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	bool found = false;
	for (const auto& id : cached_mempool_msg->mempool)
	{
		if (id == tx_id)
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		LOG_ERROR("Transaction {} not found in remote mempool", tx_id);
		return nullptr;
	}

	std::shared_ptr<Tx> original_tx;
	{
		std::scoped_lock lock(Mempool::mutex);
		const auto it = Mempool::map.find(tx_id);
		if (it != Mempool::map.end())
			original_tx = it->second.tx;
	}

	if (original_tx == nullptr)
	{
		LOG_ERROR("Transaction {} not available locally for RBF", tx_id);
		return nullptr;
	}

	if (!original_tx->signals_rbf())
	{
		LOG_ERROR("Transaction {} does not signal RBF", tx_id);
		return nullptr;
	}

	const auto pub_key = ECDSA::get_pub_key_from_priv_key(priv_key);
	auto replacement = build_rbf_replacement(original_tx, new_fee_per_byte, priv_key, pub_key);
	if (replacement == nullptr)
		return nullptr;

	LOG_INFO("Built RBF replacement {}, broadcasting", replacement->id());
	if (!NetClient::send_msg_random(TxInfoMsg(replacement)))
	{
		LOG_ERROR("No connection to send replacement transaction");
	}

	return replacement;
}

std::shared_ptr<Tx> Wallet::build_rbf_replacement(const std::shared_ptr<Tx>& original_tx,
	uint64_t new_fee_per_byte, const std::vector<uint8_t>& priv_key,
	const std::vector<uint8_t>& pub_key)
{
	uint64_t total_input = 0;
	for (const auto& tx_in : original_tx->tx_ins)
	{
		const auto utxo = UTXO::find_in_map(tx_in->to_spend);
		if (utxo == nullptr)
		{
			const auto mempool_utxo = Mempool::find_utxo_in_mempool(tx_in->to_spend);
			if (mempool_utxo == nullptr)
			{
				LOG_ERROR("Cannot find UTXO for input {}", tx_in->to_spend->tx_id);
				return nullptr;
			}
			total_input += mempool_utxo->tx_out->value;
		}
		else
		{
			total_input += utxo->tx_out->value;
		}
	}

	uint64_t payment_total = 0;
	for (uint32_t i = 0; i < original_tx->tx_outs.size(); i++)
	{
		if (i < original_tx->tx_outs.size() - 1)
			payment_total += original_tx->tx_outs[i]->value;
	}

	const uint32_t size_est = original_tx->serialize().get_size();
	const uint64_t new_total_fee = static_cast<uint64_t>(size_est) * new_fee_per_byte;

	if (total_input < payment_total + new_total_fee)
	{
		LOG_ERROR("Insufficient funds for new fee rate ({} available, {} + {} needed)",
			total_input, payment_total, new_total_fee);
		return nullptr;
	}

	const uint64_t new_change = total_input - payment_total - new_total_fee;

	std::vector<std::shared_ptr<TxOut>> new_tx_outs;
	new_tx_outs.reserve(original_tx->tx_outs.size());
	for (uint32_t i = 0; i < original_tx->tx_outs.size(); i++)
	{
		if (i < original_tx->tx_outs.size() - 1)
			new_tx_outs.push_back(std::make_shared<TxOut>(original_tx->tx_outs[i]->value,
				original_tx->tx_outs[i]->to_address));
		else
			new_tx_outs.push_back(std::make_shared<TxOut>(new_change,
				original_tx->tx_outs[i]->to_address));
	}

	std::vector<std::shared_ptr<TxIn>> new_tx_ins;
	new_tx_ins.reserve(original_tx->tx_ins.size());
	for (const auto& old_in : original_tx->tx_ins)
	{
		new_tx_ins.emplace_back(build_tx_in(priv_key, pub_key, old_in->to_spend, new_tx_outs, TxIn::SEQUENCE_RBF));
	}

	auto replacement = std::make_shared<Tx>(new_tx_ins, new_tx_outs, original_tx->lock_time);

	LOG_INFO("Built RBF replacement {} with {} total fee ({} coins/byte)",
		replacement->id(), new_total_fee, new_fee_per_byte);

	return replacement;
}

Wallet::TxStatusResponse Wallet::get_tx_status_miner(const std::string& tx_id)
{
	TxStatusResponse ret;

	{
		std::scoped_lock lock(Mempool::mutex);

		if (Mempool::map.contains(tx_id))
		{
			ret.status = TxStatus::Mempool;

			return ret;
		}
	}

	{
		std::scoped_lock lock(Chain::mutex);

		for (uint32_t height = 0; height < Chain::active_chain.size(); height++)
		{
			const auto& block = Chain::active_chain[height];
			for (const auto& tx : block->txs)
			{
				if (tx->id() == tx_id)
				{
					ret.status = TxStatus::Mined;
					ret.block_id = block->id();
					ret.block_height = height;

					return ret;
				}
			}
		}
	}

	ret.status = TxStatus::NotFound;

	return ret;
}

Wallet::TxStatusResponse Wallet::get_tx_status(const std::string& tx_id)
{
	TxStatusResponse ret;

	MsgCache::set_send_mempool_msg(nullptr);

	if (!NetClient::send_msg_random(GetMempoolMsg()))
	{
		LOG_ERROR("No connection to ask mempool");

		return ret;
	}

	auto start = Utils::get_unix_timestamp();
	std::shared_ptr<SendMempoolMsg> cached_mempool_msg;
	while (true)
	{
		cached_mempool_msg = MsgCache::get_send_mempool_msg();
		if (cached_mempool_msg != nullptr)
			break;
		if (Utils::get_unix_timestamp() - start > MsgCache::MAX_MSG_AWAIT_TIME_IN_SECS)
		{
			LOG_ERROR("Timeout on GetMempoolMsg");

			return ret;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	for (const auto& tx : cached_mempool_msg->mempool)
	{
		if (tx == tx_id)
		{
			ret.status = TxStatus::Mempool;

			return ret;
		}
	}

	MsgCache::set_send_active_chain_msg(nullptr);

	if (!NetClient::send_msg_random(GetActiveChainMsg()))
	{
		LOG_ERROR("No connection to ask active chain");

		return ret;
	}

	start = Utils::get_unix_timestamp();
	std::shared_ptr<SendActiveChainMsg> cached_chain_msg;
	while (true)
	{
		cached_chain_msg = MsgCache::get_send_active_chain_msg();
		if (cached_chain_msg != nullptr)
			break;
		if (Utils::get_unix_timestamp() - start > MsgCache::MAX_MSG_AWAIT_TIME_IN_SECS)
		{
			LOG_ERROR("Timeout on GetActiveChainMsg");

			return ret;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	for (uint32_t height = 0; height < cached_chain_msg->active_chain.size(); height++)
	{
		const auto& block = cached_chain_msg->active_chain[height];
		for (const auto& tx : block->txs)
		{
			if (tx->id() == tx_id)
			{
				ret.status = TxStatus::Mined;
				ret.block_id = block->id();
				ret.block_height = height;

				return ret;
			}
		}
	}

	ret.status = TxStatus::NotFound;

	return ret;
}

void Wallet::print_tx_status(const std::string& tx_id)
{
	auto response = get_tx_status(tx_id);
	switch (response.status)
	{
		case TxStatus::Mempool:
		{
			LOG_INFO("Transaction {} is in mempool", tx_id);

			break;
		}
		case TxStatus::Mined:
		{
			LOG_INFO("Transaction {} is mined in {} at height {}", tx_id, response.block_id, response.block_height);

			break;
		}
		case TxStatus::NotFound:
		{
			LOG_INFO("Transaction {} not found", tx_id);

			break;
		}
	}
}

static uint64_t sum_utxo_values(const std::vector<std::shared_ptr<UTXO>>& utxos)
{
	uint64_t value = 0;
	for (const auto& utxo : utxos)
		value += utxo->tx_out->value;
	return value;
}

uint64_t Wallet::get_balance_miner(const std::string& address)
{
	return sum_utxo_values(find_utxos_for_address_miner(address));
}

uint64_t Wallet::get_balance(const std::string& address)
{
	return sum_utxo_values(find_utxos_for_address(address));
}

void Wallet::print_balance(const std::string& address)
{
	uint64_t balance = get_balance(address);
	LOG_INFO("Address {} holds {} coins", address, balance);
}

std::vector<std::shared_ptr<UTXO>> Wallet::branch_and_bound_select(const std::vector<std::shared_ptr<UTXO>>& utxos,
	uint64_t target, uint64_t cost_of_change)
{
	std::vector<std::shared_ptr<UTXO>> sorted_utxos(utxos.begin(), utxos.end());
	std::ranges::sort(sorted_utxos,
		[](const std::shared_ptr<UTXO>& a, const std::shared_ptr<UTXO>& b)
	{
		return a->tx_out->value > b->tx_out->value;
	});

	std::vector<uint64_t> suffix_sums(sorted_utxos.size() + 1, 0);
	for (int64_t i = static_cast<int64_t>(sorted_utxos.size()) - 1; i >= 0; i--)
		suffix_sums[i] = suffix_sums[i + 1] + sorted_utxos[i]->tx_out->value;

	uint64_t current_value = 0;
	std::vector<bool> inclusion(sorted_utxos.size(), false);
	std::vector<bool> best_selection;
	uint64_t best_waste = std::numeric_limits<uint64_t>::max();

	constexpr uint32_t MAX_TRIES = 100000;
	uint32_t tries = 0;
	size_t depth = 0;
	bool backtrack = false;

	while (tries < MAX_TRIES)
	{
		tries++;

		if (depth >= sorted_utxos.size())
		{
			backtrack = true;
		}

		if (!backtrack)
		{
			inclusion[depth] = true;
			current_value += sorted_utxos[depth]->tx_out->value;

			if (current_value >= target)
			{
				uint64_t waste = current_value - target;
				if (waste < best_waste)
				{
					best_waste = waste;
					best_selection = inclusion;
					best_selection.resize(sorted_utxos.size(), false);
				}
				if (waste == 0)
					break;
				backtrack = true;
			}
			else if (depth + 1 < sorted_utxos.size() &&
				current_value + suffix_sums[depth + 1] >= target)
			{
				depth++;
				continue;
			}
			else
			{
				backtrack = true;
			}
		}

		if (backtrack)
		{
			if (inclusion[depth])
			{
				current_value -= sorted_utxos[depth]->tx_out->value;
				inclusion[depth] = false;
			}

			if (depth + 1 < sorted_utxos.size() &&
				current_value + suffix_sums[depth + 1] >= target)
			{
				depth++;
				backtrack = false;
				continue;
			}

			while (depth > 0)
			{
				depth--;
				if (inclusion[depth])
				{
					current_value -= sorted_utxos[depth]->tx_out->value;
					inclusion[depth] = false;

					if (depth + 1 < sorted_utxos.size() &&
						current_value + suffix_sums[depth + 1] >= target)
					{
						depth++;
						backtrack = false;
						break;
					}
				}
			}

			if (backtrack)
				break;
		}
	}

	if (best_selection.empty())
		return {};

	if (best_waste > cost_of_change)
		return {};

	std::vector<std::shared_ptr<UTXO>> result;
	for (size_t i = 0; i < sorted_utxos.size(); i++)
	{
		if (best_selection[i])
			result.push_back(sorted_utxos[i]);
	}

	return result;
}

std::shared_ptr<Tx> Wallet::build_tx_from_utxos(std::vector<std::shared_ptr<UTXO>>& utxos, uint64_t value, uint64_t fee,
	const std::string& address, const std::string& change_address,
	const std::vector<uint8_t>& priv_key, const std::vector<uint8_t>& pub_key,
	int64_t lock_time)
{
	const uint32_t total_size_est = 300;
	const uint64_t total_fee_est = total_size_est * fee;
	const uint64_t target = value + total_fee_est;

	const uint64_t cost_of_change = 34 * fee;

	auto selected_utxos = branch_and_bound_select(utxos, target, cost_of_change);
	bool exact_match = !selected_utxos.empty();

	if (selected_utxos.empty())
	{
		std::vector<std::shared_ptr<UTXO>> sorted_utxos(utxos.begin(), utxos.end());
		std::ranges::sort(sorted_utxos,
			[](const std::shared_ptr<UTXO>& a, const std::shared_ptr<UTXO>& b) -> bool
		{
			return a->tx_out->value > b->tx_out->value;
		});

		uint64_t in_sum = 0;
		for (const auto& coin : sorted_utxos)
		{
			selected_utxos.push_back(coin);
			in_sum += coin->tx_out->value;
			if (in_sum > target)
				break;
		}
		if (in_sum <= target)
		{
			LOG_ERROR("Not enough coins");

			return nullptr;
		}
	}

	uint64_t in_sum = 0;
	for (const auto& coin : selected_utxos)
		in_sum += coin->tx_out->value;

	std::vector<std::shared_ptr<TxOut>> tx_outs;
	const auto tx_out = std::make_shared<TxOut>(value, address);
	tx_outs.push_back(tx_out);

	if (!exact_match)
	{
		uint64_t change = in_sum - value - total_fee_est;
		const auto tx_out_change = std::make_shared<TxOut>(change, change_address);
		tx_outs.push_back(tx_out_change);
	}

	std::vector<std::shared_ptr<TxIn>> tx_ins;
	tx_ins.reserve(selected_utxos.size());
	for (const auto& selected_coin : selected_utxos)
	{
		tx_ins.emplace_back(build_tx_in(priv_key, pub_key, selected_coin->tx_out_point, tx_outs));
	}
	auto tx = std::make_shared<Tx>(tx_ins, tx_outs, lock_time);
	const uint32_t tx_size = tx->serialize().get_size();
	const uint64_t real_fee = static_cast<uint64_t>(tx_size) * fee;
	LOG_INFO("Built transaction {} with {} total fee ({} coins/byte)", tx->id(), real_fee, fee);
	return tx;
}

std::shared_ptr<Tx> Wallet::build_tx_miner(uint64_t value, uint64_t fee, const std::string& address,
	const std::vector<uint8_t>& priv_key, int64_t lock_time)
{
	const auto pub_key = ECDSA::get_pub_key_from_priv_key(priv_key);
	const auto my_address = pub_key_to_address(pub_key);
	auto my_coins = find_utxos_for_address_miner(my_address);
	if (my_coins.empty())
	{
		LOG_ERROR("No coins found");
		return nullptr;
	}
	return build_tx_from_utxos(my_coins, value, fee, address, my_address, priv_key, pub_key, lock_time);
}

std::shared_ptr<Tx> Wallet::build_tx(uint64_t value, uint64_t fee, const std::string& address,
	const std::vector<uint8_t>& priv_key, int64_t lock_time)
{
	const auto pub_key = ECDSA::get_pub_key_from_priv_key(priv_key);
	const auto my_address = pub_key_to_address(pub_key);
	auto my_coins = find_utxos_for_address(my_address);
	if (my_coins.empty())
	{
		LOG_ERROR("No coins found");
		return nullptr;
	}
	return build_tx_from_utxos(my_coins, value, fee, address, my_address, priv_key, pub_key, lock_time);
}

std::vector<std::shared_ptr<UTXO>> Wallet::find_utxos_for_address_miner(const std::string& address)
{
	std::vector<std::shared_ptr<UTXO>> utxos;
	{
		std::scoped_lock lock(UTXO::mutex);

		for (const auto& v : UTXO::map | std::views::values)
		{
			if (v->tx_out->to_address == address)
			{
				utxos.push_back(v);
			}
		}
	}
	return utxos;
}

std::vector<std::shared_ptr<UTXO>> Wallet::find_utxos_for_address(const std::string& address)
{
	MsgCache::set_send_utxos_msg(nullptr);

	if (!NetClient::send_msg_random(GetUTXOsMsg()))
	{
		LOG_ERROR("No connection to ask UTXO set");

		return {};
	}

	const auto start = Utils::get_unix_timestamp();
	std::shared_ptr<SendUTXOsMsg> cached_utxos_msg;
	while (true)
	{
		cached_utxos_msg = MsgCache::get_send_utxos_msg();
		if (cached_utxos_msg != nullptr)
			break;
		if (Utils::get_unix_timestamp() - start > MsgCache::MAX_MSG_AWAIT_TIME_IN_SECS)
		{
			LOG_ERROR("Timeout on GetUTXOsMsg");

			return {};
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	std::vector<std::shared_ptr<UTXO>> utxos;
	for (const auto& v : cached_utxos_msg->utxo_map | std::views::values)
	{
		if (v->tx_out->to_address == address)
		{
			utxos.push_back(v);
		}
	}
	return utxos;
}

std::tuple<std::shared_ptr<HDWallet>, std::string> Wallet::init_hd_wallet(const std::string& wallet_path)
{
	Wallet::hd_wallet_path = wallet_path;

	std::shared_ptr<HDWallet> hd_wallet;
	try
	{
		auto loaded = HDWallet::load(wallet_path);
		hd_wallet = std::make_shared<HDWallet>(std::move(loaded));
		LOG_INFO("Loaded HD wallet from {}", wallet_path);
	}
	catch (const std::exception&)
	{
		LOG_INFO("Generating new HD wallet {}", wallet_path);
		auto created = HDWallet::create();
		created.save(wallet_path);
		hd_wallet = std::make_shared<HDWallet>(std::move(created));
	}

	if (hd_wallet->get_external_index() == 0)
		hd_wallet->get_new_address();

	const auto address = hd_wallet->get_primary_address();

	static std::once_flag print_flag;
	std::call_once(print_flag, [&address]()
	{
		LOG_INFO("Your HD wallet primary address is {}", address);
	});

	return { hd_wallet, address };
}

std::tuple<std::shared_ptr<HDWallet>, std::string> Wallet::init_hd_wallet()
{
	return init_hd_wallet(hd_wallet_path);
}

std::shared_ptr<Tx> Wallet::send_value_hd_miner(uint64_t value, uint64_t fee, const std::string& address,
	HDWallet& hd_wallet, int64_t lock_time)
{
	auto tx = build_tx_hd_miner(value, fee, address, hd_wallet, lock_time);
	if (tx == nullptr)
		return nullptr;
	LOG_INFO("Built HD transaction {}, adding to mempool", tx->id());
	Mempool::add_tx_to_mempool(tx);
	NetClient::send_msg_random(TxInfoMsg(tx));

	hd_wallet.save(hd_wallet_path);

	return tx;
}

std::shared_ptr<Tx> Wallet::send_value_hd(uint64_t value, uint64_t fee, const std::string& address,
	HDWallet& hd_wallet, int64_t lock_time)
{
	auto tx = build_tx_hd(value, fee, address, hd_wallet, lock_time);
	if (tx == nullptr)
		return nullptr;
	LOG_INFO("Built HD transaction {}, broadcasting", tx->id());
	if (!NetClient::send_msg_random(TxInfoMsg(tx)))
	{
		LOG_ERROR("No connection to send transaction");
	}

	hd_wallet.save(hd_wallet_path);

	return tx;
}

std::shared_ptr<Tx> Wallet::build_tx_hd_miner(uint64_t value, uint64_t fee, const std::string& address,
	HDWallet& hd_wallet, int64_t lock_time)
{
	auto my_coins = find_utxos_for_hd_wallet_miner(hd_wallet);
	if (my_coins.empty())
	{
		LOG_ERROR("No coins found in HD wallet");
		return nullptr;
	}
	return build_tx_from_utxos_hd(my_coins, value, fee, address, hd_wallet, lock_time);
}

std::shared_ptr<Tx> Wallet::build_tx_hd(uint64_t value, uint64_t fee, const std::string& address,
	HDWallet& hd_wallet, int64_t lock_time)
{
	auto my_coins = find_utxos_for_hd_wallet(hd_wallet);
	if (my_coins.empty())
	{
		LOG_ERROR("No coins found in HD wallet");
		return nullptr;
	}
	return build_tx_from_utxos_hd(my_coins, value, fee, address, hd_wallet, lock_time);
}

std::shared_ptr<Tx> Wallet::build_tx_from_utxos_hd(std::vector<std::shared_ptr<UTXO>>& utxos,
	uint64_t value, uint64_t fee, const std::string& address,
	HDWallet& hd_wallet, int64_t lock_time)
{
	const uint32_t total_size_est = 300;
	const uint64_t total_fee_est = total_size_est * fee;
	const uint64_t target = value + total_fee_est;

	const uint64_t cost_of_change = 34 * fee;

	auto selected_utxos = branch_and_bound_select(utxos, target, cost_of_change);
	bool exact_match = !selected_utxos.empty();

	if (selected_utxos.empty())
	{
		std::vector<std::shared_ptr<UTXO>> sorted_utxos(utxos.begin(), utxos.end());
		std::ranges::sort(sorted_utxos,
			[](const std::shared_ptr<UTXO>& a, const std::shared_ptr<UTXO>& b) -> bool
		{
			return a->tx_out->value > b->tx_out->value;
		});

		uint64_t in_sum = 0;
		for (const auto& coin : sorted_utxos)
		{
			selected_utxos.push_back(coin);
			in_sum += coin->tx_out->value;
			if (in_sum > target)
				break;
		}
		if (in_sum <= target)
		{
			LOG_ERROR("Not enough coins");
			return nullptr;
		}
	}

	uint64_t in_sum = 0;
	for (const auto& coin : selected_utxos)
		in_sum += coin->tx_out->value;

	const auto change_address = hd_wallet.get_change_address();

	std::vector<std::shared_ptr<TxOut>> tx_outs;
	const auto tx_out = std::make_shared<TxOut>(value, address);
	tx_outs.push_back(tx_out);

	if (!exact_match)
	{
		uint64_t change = in_sum - value - total_fee_est;
		const auto tx_out_change = std::make_shared<TxOut>(change, change_address);
		tx_outs.push_back(tx_out_change);
	}

	std::vector<std::shared_ptr<TxIn>> tx_ins;
	tx_ins.reserve(selected_utxos.size());
	for (const auto& selected_coin : selected_utxos)
	{
		const auto& utxo_address = selected_coin->tx_out->to_address;

		std::vector<uint8_t> priv_key;
		std::vector<uint8_t> pub_key;
		if (!hd_wallet.get_keys_for_address(utxo_address, priv_key, pub_key))
		{
			LOG_ERROR("HD wallet does not own address {} for UTXO", utxo_address);
			return nullptr;
		}

		tx_ins.emplace_back(build_tx_in(priv_key, pub_key, selected_coin->tx_out_point, tx_outs));
	}

	auto tx = std::make_shared<Tx>(tx_ins, tx_outs, lock_time);
	const uint32_t tx_size = tx->serialize().get_size();
	const uint64_t real_fee = static_cast<uint64_t>(tx_size) * fee;
	LOG_INFO("Built HD transaction {} with {} total fee ({} coins/byte)", tx->id(), real_fee, fee);
	return tx;
}

std::vector<std::shared_ptr<UTXO>> Wallet::find_utxos_for_hd_wallet_miner(HDWallet& hd_wallet)
{
	const auto addresses = hd_wallet.get_all_addresses();
	std::vector<std::shared_ptr<UTXO>> utxos;
	{
		std::scoped_lock lock(UTXO::mutex);

		for (const auto& v : UTXO::map | std::views::values)
		{
			for (const auto& addr : addresses)
			{
				if (v->tx_out->to_address == addr)
				{
					utxos.push_back(v);
					break;
				}
			}
		}
	}
	return utxos;
}

std::vector<std::shared_ptr<UTXO>> Wallet::find_utxos_for_hd_wallet(HDWallet& hd_wallet)
{
	MsgCache::set_send_utxos_msg(nullptr);

	if (!NetClient::send_msg_random(GetUTXOsMsg()))
	{
		LOG_ERROR("No connection to ask UTXO set");
		return {};
	}

	const auto start = Utils::get_unix_timestamp();
	std::shared_ptr<SendUTXOsMsg> cached_utxos_msg;
	while (true)
	{
		cached_utxos_msg = MsgCache::get_send_utxos_msg();
		if (cached_utxos_msg != nullptr)
			break;
		if (Utils::get_unix_timestamp() - start > MsgCache::MAX_MSG_AWAIT_TIME_IN_SECS)
		{
			LOG_ERROR("Timeout on GetUTXOsMsg");
			return {};
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	const auto addresses = hd_wallet.get_all_addresses();
	std::vector<std::shared_ptr<UTXO>> utxos;
	for (const auto& v : cached_utxos_msg->utxo_map | std::views::values)
	{
		for (const auto& addr : addresses)
		{
			if (v->tx_out->to_address == addr)
			{
				utxos.push_back(v);
				break;
			}
		}
	}
	return utxos;
}
