#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "core/enums.hpp"
#include "core/tx.hpp"
#include "core/tx_in.hpp"
#include "core/tx_out.hpp"
#include "core/tx_out_point.hpp"

class UnspentTxOut;

class Wallet
{
public:
	static std::string pub_key_to_address(const std::vector<uint8_t>& pub_key);

	static std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string>
		get_wallet(const std::string& wallet_path);
	static void print_wallet_address(const std::string& wallet_path);
	static std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> init_wallet(
		const std::string& wallet_path);
	static std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> init_wallet();

	static std::shared_ptr<TxIn> build_tx_in(const std::vector<uint8_t>& priv_key,
		const std::vector<uint8_t>& pub_key,
		const std::shared_ptr<TxOutPoint>& tx_out_point,
		const std::vector<std::shared_ptr<TxOut>>& tx_outs);
	static std::shared_ptr<Tx> send_value_miner(uint64_t value, uint64_t fee, const std::string& address,
		const std::vector<uint8_t>& priv_key);
	static std::shared_ptr<Tx> send_value(uint64_t value, uint64_t fee, const std::string& address,
		const std::vector<uint8_t>& priv_key);

	struct TxStatusResponse
	{
		TxStatus status = TxStatus::NotFound;
		std::string block_id;
		int64_t block_height = -1;
	};

	static TxStatusResponse get_tx_status_miner(const std::string& tx_id);
	static TxStatusResponse get_tx_status(const std::string& tx_id);
	static void print_tx_status(const std::string& tx_id);

	static uint64_t get_balance_miner(const std::string& address);
	static uint64_t get_balance(const std::string& address);
	static void print_balance(const std::string& address);

private:
	static constexpr char DEFAULT_WALLET_PATH[] = "wallet.dat";
	static std::string wallet_path;

	static std::shared_ptr<Tx> build_tx_from_utxos(std::vector<std::shared_ptr<UnspentTxOut>>& utxos, uint64_t value,
		uint64_t fee, const std::string& address,
		const std::string& change_address,
		const std::vector<uint8_t>& priv_key,
		const std::vector<uint8_t>& pub_key);

	static std::shared_ptr<Tx> build_tx_miner(uint64_t value, uint64_t fee, const std::string& address,
		const std::vector<uint8_t>& priv_key);
	static std::shared_ptr<Tx> build_tx(uint64_t value, uint64_t fee, const std::string& address,
		const std::vector<uint8_t>& priv_key);

	static std::vector<std::shared_ptr<UnspentTxOut>> find_utxos_for_address_miner(const std::string& address);
	static std::vector<std::shared_ptr<UnspentTxOut>> find_utxos_for_address(const std::string& address);
};
