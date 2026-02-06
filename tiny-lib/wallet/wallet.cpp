#include "wallet/wallet.hpp"

#include <chrono>
#include <fstream>
#include <iterator>
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

std::string Wallet::wallet_path = DEFAULT_WALLET_PATH;

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
	int32_t sequence = -1;

	const auto spend_msg = MsgSerializer::build_spend_msg(tx_out_point, pub_key, sequence, tx_outs);
	auto unlock_sig = ECDSA::sign_msg(spend_msg, priv_key);

	return std::make_shared<TxIn>(tx_out_point, unlock_sig, pub_key, sequence);
}

std::shared_ptr<Tx> Wallet::send_value_miner(uint64_t value, uint64_t fee, const std::string& address,
	const std::vector<uint8_t>& priv_key)
{
	auto tx = build_tx_miner(value, fee, address, priv_key);
	if (tx == nullptr)
		return nullptr;
	LOG_INFO("Built transaction {}, adding to mempool", tx->id());
	Mempool::add_tx_to_mempool(tx);
	NetClient::send_msg_random(TxInfoMsg(tx));

	return tx;
}

std::shared_ptr<Tx> Wallet::send_value(uint64_t value, uint64_t fee, const std::string& address,
	const std::vector<uint8_t>& priv_key)
{
	auto tx = build_tx(value, fee, address, priv_key);
	if (tx == nullptr)
		return nullptr;
	LOG_INFO("Built transaction {}, broadcasting", tx->id());
	if (!NetClient::send_msg_random(TxInfoMsg(tx)))
	{
		LOG_ERROR("No connection to send transaction");
	}

	return tx;
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

std::shared_ptr<Tx> Wallet::build_tx_from_utxos(std::vector<std::shared_ptr<UTXO>>& utxos, uint64_t value, uint64_t fee,
	const std::string& address, const std::string& change_address,
	const std::vector<uint8_t>& priv_key, const std::vector<uint8_t>& pub_key)
{
	std::ranges::sort(utxos,
		[](const std::shared_ptr<UTXO>& a, const std::shared_ptr<UTXO>& b) -> bool
	{
		if (a->height != b->height)
			return a->height < b->height;
		return a->tx_out->value < b->tx_out->value;
	});
	std::vector<std::shared_ptr<UTXO>> selected_utxos;
	uint64_t in_sum = 0;
	const uint32_t total_size_est = 300;
	const uint64_t total_fee_est = total_size_est * fee;
	for (const auto& coin : utxos)
	{
		selected_utxos.push_back(coin);
		in_sum += coin->tx_out->value;
		if (in_sum > value + total_fee_est)
		{
			break;
		}
	}
	if (in_sum <= value + total_fee_est)
	{
		LOG_ERROR("Not enough coins");

		return nullptr;
	}
	const auto tx_out = std::make_shared<TxOut>(value, address);
	uint64_t change = in_sum - value - total_fee_est;
	const auto tx_out_change = std::make_shared<TxOut>(change, change_address);
	std::vector tx_outs{ tx_out, tx_out_change };
	std::vector<std::shared_ptr<TxIn>> tx_ins;
	tx_ins.reserve(selected_utxos.size());
	for (const auto& selected_coin : selected_utxos)
	{
		tx_ins.emplace_back(build_tx_in(priv_key, pub_key, selected_coin->tx_out_point, tx_outs));
	}
	auto tx = std::make_shared<Tx>(tx_ins, tx_outs, 0);
	const uint32_t tx_size = tx->serialize().get_size();
	const uint64_t real_fee = static_cast<uint64_t>(tx_size) * fee;
	LOG_INFO("Built transaction {} with {} total fee ({} coins/byte)", tx->id(), real_fee, fee);
	return tx;
}

std::shared_ptr<Tx> Wallet::build_tx_miner(uint64_t value, uint64_t fee, const std::string& address,
	const std::vector<uint8_t>& priv_key)
{
	const auto pub_key = ECDSA::get_pub_key_from_priv_key(priv_key);
	const auto my_address = pub_key_to_address(pub_key);
	auto my_coins = find_utxos_for_address_miner(my_address);
	if (my_coins.empty())
	{
		LOG_ERROR("No coins found");
		return nullptr;
	}
	return build_tx_from_utxos(my_coins, value, fee, address, my_address, priv_key, pub_key);
}

std::shared_ptr<Tx> Wallet::build_tx(uint64_t value, uint64_t fee, const std::string& address,
	const std::vector<uint8_t>& priv_key)
{
	const auto pub_key = ECDSA::get_pub_key_from_priv_key(priv_key);
	const auto my_address = pub_key_to_address(pub_key);
	auto my_coins = find_utxos_for_address(my_address);
	if (my_coins.empty())
	{
		LOG_ERROR("No coins found");
		return nullptr;
	}
	return build_tx_from_utxos(my_coins, value, fee, address, my_address, priv_key, pub_key);
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
