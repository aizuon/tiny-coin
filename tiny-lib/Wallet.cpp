#include "pch.hpp"
#include "Wallet.hpp"

#include <chrono>
#include <fstream>
#include <iterator>
#include <ranges>
#include <thread>
#include <unordered_set>

#include "Base58.hpp"
#include "Chain.hpp"
#include "ECDSA.hpp"
#include "GetActiveChainMsg.hpp"
#include "GetMempoolMsg.hpp"
#include "GetUTXOsMsg.hpp"
#include "Log.hpp"
#include "Mempool.hpp"
#include "MsgCache.hpp"
#include "MsgSerializer.hpp"
#include "NetClient.hpp"
#include "RIPEMD160.hpp"
#include "SendActiveChainMsg.hpp"
#include "SendMempoolMsg.hpp"
#include "SendUTXOsMsg.hpp"
#include "SHA256.hpp"
#include "Tx.hpp"
#include "TxInfoMsg.hpp"
#include "UnspentTxOut.hpp"
#include "Utils.hpp"

std::string Wallet::WalletPath = DefaultWalletPath;

std::string Wallet::PubKeyToAddress(const std::vector<uint8_t>& pub_key)
{
	const auto sha256 = SHA256::HashBinary(pub_key);

	auto ripe = RIPEMD160::HashBinary(sha256);

	ripe.insert(ripe.begin(), 0x00);

	auto sha256d = SHA256::DoubleHashBinary(ripe);

	std::vector checksum(sha256d.begin(), sha256d.begin() + 4);

	ripe.insert(ripe.end(), checksum.begin(), checksum.end());

	return PubKeyHashVersion + Base58::Encode(ripe);
}

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> Wallet::GetWallet(const std::string& wallet_path)
{
	std::vector<uint8_t> priv_key;
	std::vector<uint8_t> pub_key;
	std::string address;

	std::ifstream wallet_in(wallet_path, std::ios::binary);
	if (wallet_in.good())
	{
		priv_key = std::vector<uint8_t>(std::istreambuf_iterator(wallet_in), {});
		pub_key = ECDSA::GetPubKeyFromPrivKey(priv_key);
		address = PubKeyToAddress(pub_key);
		wallet_in.close();
	}
	else
	{
		LOG_INFO("Generating new wallet {}", wallet_path);

		auto [privKey2, pubKey2] = ECDSA::Generate();
		priv_key = privKey2;
		pub_key = pubKey2;
		address = PubKeyToAddress(pub_key);

		std::ofstream wallet_out(wallet_path, std::ios::binary);
		wallet_out.write(reinterpret_cast<const char*>(priv_key.data()), priv_key.size());
		wallet_out.flush();
		wallet_out.close();
	}

	return { priv_key, pub_key, address };
}

void Wallet::PrintWalletAddress(const std::string& wallet_path)
{
	const auto [priv_key, pub_key, address] = GetWallet(wallet_path);

	LOG_INFO("Wallet {} belongs to address {}", wallet_path, address);
}

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> Wallet::InitWallet(const std::string& wallet_path)
{
	WalletPath = wallet_path;

	const auto [priv_key, pub_key, address] = GetWallet(WalletPath);

	static bool printedAddress = false;
	if (!printedAddress)
	{
		printedAddress = true;

		LOG_INFO("Your address is {}", address);
	}

	return { priv_key, pub_key, address };
}

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> Wallet::InitWallet()
{
	return InitWallet(WalletPath);
}

std::shared_ptr<TxIn> Wallet::BuildTxIn(const std::vector<uint8_t>& priv_key,
                                        std::shared_ptr<TxOutPoint> tx_out_point,
                                        const std::vector<std::shared_ptr<TxOut>>& tx_outs)
{
	int32_t sequence = -1;

	auto pub_key = ECDSA::GetPubKeyFromPrivKey(priv_key);
	const auto spend_msg = MsgSerializer::BuildSpendMsg(tx_out_point, pub_key, sequence, tx_outs);
	auto unlock_sig = ECDSA::SignMsg(spend_msg, priv_key);

	return std::make_shared<TxIn>(tx_out_point, unlock_sig, pub_key, sequence);
}

std::shared_ptr<Tx> Wallet::SendValue_Miner(uint64_t value, uint64_t fee, const std::string& address,
                                            const std::vector<uint8_t>& priv_key)
{
	auto tx = BuildTx_Miner(value, fee, address, priv_key);
	if (tx == nullptr)
		return nullptr;
	LOG_INFO("Built transaction {}, adding to mempool", tx->Id());
	Mempool::AddTxToMempool(tx);
	NetClient::SendMsgRandom(TxInfoMsg(tx));

	return tx;
}

std::shared_ptr<Tx> Wallet::SendValue(uint64_t value, uint64_t fee, const std::string& address,
                                      const std::vector<uint8_t>& priv_key)
{
	auto tx = BuildTx(value, fee, address, priv_key);
	if (tx == nullptr)
		return nullptr;
	LOG_INFO("Built transaction {}, broadcasting", tx->Id());
	if (!NetClient::SendMsgRandom(TxInfoMsg(tx)))
	{
		LOG_ERROR("No connection to send transaction");
	}

	return tx;
}

Wallet::TxStatusResponse Wallet::GetTxStatus_Miner(const std::string& tx_id)
{
	TxStatusResponse ret;

	{
		std::scoped_lock lock(Mempool::Mutex);

		for (const auto& tx : Mempool::Map | std::views::keys)
		{
			if (tx == tx_id)
			{
				ret.Status = TxStatus::Mempool;

				return ret;
			}
		}
	}

	{
		std::scoped_lock lock(Chain::Mutex);

		for (uint32_t height = 0; height < Chain::ActiveChain.size(); height++)
		{
			const auto& block = Chain::ActiveChain[height];
			for (const auto& tx : block->Txs)
			{
				if (tx->Id() == tx_id)
				{
					ret.Status = TxStatus::Mined;
					ret.BlockId = block->Id();
					ret.BlockHeight = height;

					return ret;
				}
			}
		}
	}

	ret.Status = TxStatus::NotFound;

	return ret;
}

Wallet::TxStatusResponse Wallet::GetTxStatus(const std::string& tx_id)
{
	TxStatusResponse ret;

	if (MsgCache::SendMempoolMsg != nullptr)
		MsgCache::SendMempoolMsg = nullptr;

	if (!NetClient::SendMsgRandom(GetMempoolMsg()))
	{
		LOG_ERROR("No connection to ask mempool");

		return ret;
	}

	auto start = Utils::GetUnixTimestamp();
	while (MsgCache::SendMempoolMsg == nullptr)
	{
		if (Utils::GetUnixTimestamp() - start > MsgCache::MAX_MSG_AWAIT_TIME_IN_SECS)
		{
			LOG_ERROR("Timeout on GetMempoolMsg");

			return ret;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	for (const auto& tx : MsgCache::SendMempoolMsg->Mempool)
	{
		if (tx == tx_id)
		{
			ret.Status = TxStatus::Mempool;

			return ret;
		}
	}

	if (MsgCache::SendActiveChainMsg != nullptr)
		MsgCache::SendActiveChainMsg = nullptr;

	if (!NetClient::SendMsgRandom(GetActiveChainMsg()))
	{
		LOG_ERROR("No connection to ask active chain");

		return ret;
	}

	start = Utils::GetUnixTimestamp();
	while (MsgCache::SendActiveChainMsg == nullptr)
	{
		if (Utils::GetUnixTimestamp() - start > MsgCache::MAX_MSG_AWAIT_TIME_IN_SECS)
		{
			LOG_ERROR("Timeout on GetActiveChainMsg");

			return ret;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	for (uint32_t height = 0; height < MsgCache::SendActiveChainMsg->ActiveChain.size(); height++)
	{
		const auto& block = MsgCache::SendActiveChainMsg->ActiveChain[height];
		for (const auto& tx : block->Txs)
		{
			if (tx->Id() == tx_id)
			{
				ret.Status = TxStatus::Mined;
				ret.BlockId = block->Id();
				ret.BlockHeight = height;

				return ret;
			}
		}
	}

	ret.Status = TxStatus::NotFound;

	return ret;
}

void Wallet::PrintTxStatus(const std::string& tx_id)
{
	auto response = GetTxStatus(tx_id);
	switch (response.Status)
	{
	case TxStatus::Mempool:
		{
			LOG_INFO("Transaction {} is in mempool", tx_id);

			break;
		}
	case TxStatus::Mined:
		{
			LOG_INFO("Transaction {} is mined in {} at height {}", tx_id, response.BlockId, response.BlockHeight);

			break;
		}
	case TxStatus::NotFound:
		{
			LOG_INFO("Transaction {} not found", tx_id);

			break;
		}
	}
}

uint64_t Wallet::GetBalance_Miner(const std::string& address)
{
	const auto utxos = FindUTXOsForAddress_Miner(address);
	uint64_t value = 0;
	for (const auto& utxo : utxos)
		value += utxo->TxOut->Value;

	return value;
}

uint64_t Wallet::GetBalance(const std::string& address)
{
	const auto utxos = FindUTXOsForAddress(address);
	uint64_t value = 0;
	for (const auto& utxo : utxos)
		value += utxo->TxOut->Value;

	return value;
}

void Wallet::PrintBalance(const std::string& address)
{
	uint64_t balance = GetBalance(address);
	LOG_INFO("Address {} holds {} coins", address, balance);
}

std::shared_ptr<Tx> Wallet::BuildTxFromUTXOs(std::vector<std::shared_ptr<UTXO>>& utxos, uint64_t value, uint64_t fee,
                                             const std::string& address, const std::string& change_address,
                                             const std::vector<uint8_t>& priv_key)
{
	std::ranges::sort(utxos,
	                  [](const std::shared_ptr<UTXO>& a, const std::shared_ptr<UTXO>& b) -> bool
	                  {
		                  return a->TxOut->Value < b->TxOut->Value;
	                  });
	std::ranges::sort(utxos,
	                  [](const std::shared_ptr<UTXO>& a, const std::shared_ptr<UTXO>& b) -> bool
	                  {
		                  return a->Height < b->Height;
	                  });
	std::unordered_set<std::shared_ptr<UTXO>> selected_utxos;
	uint64_t in_sum = 0;
	const uint32_t total_size_est = 300;
	const uint64_t total_fee_est = total_size_est * fee;
	for (const auto& coin : utxos)
	{
		selected_utxos.insert(selected_utxos.end(), coin);
		for (const auto& selected_coin : selected_utxos)
		{
			in_sum += selected_coin->TxOut->Value;
		}
		if (in_sum <= value + total_fee_est)
		{
			in_sum = 0;
		}
		else
		{
			break;
		}
	}
	if (in_sum == 0)
	{
		LOG_ERROR("Not enough coins");

		return nullptr;
	}
	const auto tx_out = std::make_shared<TxOut>(value, address);
	uint64_t change = in_sum - value - total_fee_est;
	const auto tx_out_change = std::make_shared<TxOut>(change, change_address);
	std::vector tx_outs{ tx_out, tx_out_change };
	std::vector<std::shared_ptr<TxIn>> txIns;
	txIns.reserve(selected_utxos.size());
	for (const auto& selected_coin : selected_utxos)
	{
		txIns.emplace_back(BuildTxIn(priv_key, selected_coin->TxOutPoint, tx_outs));
	}
	auto tx = std::make_shared<Tx>(txIns, tx_outs, 0);
	const uint32_t tx_size = tx->Serialize().GetBuffer().size();
	const uint32_t real_fee = total_fee_est / tx_size;
	LOG_INFO("Built transaction {} with {} coins/byte fee", tx->Id(), real_fee);
	return tx;
}

std::shared_ptr<Tx> Wallet::BuildTx_Miner(uint64_t value, uint64_t fee, const std::string& address,
                                          const std::vector<uint8_t>& priv_key)
{
	const auto pub_key = ECDSA::GetPubKeyFromPrivKey(priv_key);
	const auto my_address = PubKeyToAddress(pub_key);
	auto my_coins = FindUTXOsForAddress_Miner(my_address);
	if (my_coins.empty())
	{
		LOG_ERROR("No coins found");

		return nullptr;
	}

	return BuildTxFromUTXOs(my_coins, value, fee, address, my_address, priv_key);
}

std::shared_ptr<Tx> Wallet::BuildTx(uint64_t value, uint64_t fee, const std::string& address,
                                    const std::vector<uint8_t>& priv_key)
{
	const auto pub_key = ECDSA::GetPubKeyFromPrivKey(priv_key);
	const auto my_address = PubKeyToAddress(pub_key);
	auto my_coins = FindUTXOsForAddress(my_address);
	if (my_coins.empty())
	{
		LOG_ERROR("No coins found");

		return nullptr;
	}
	return BuildTxFromUTXOs(my_coins, value, fee, address, my_address, priv_key);
}

std::vector<std::shared_ptr<UTXO>> Wallet::FindUTXOsForAddress_Miner(const std::string& address)
{
	std::vector<std::shared_ptr<UTXO>> utxos;
	{
		std::scoped_lock lock(UTXO::Mutex);

		for (const auto& v : UTXO::Map | std::views::values)
		{
			if (v->TxOut->ToAddress == address)
			{
				utxos.push_back(v);
			}
		}
	}
	return utxos;
}

std::vector<std::shared_ptr<UTXO>> Wallet::FindUTXOsForAddress(const std::string& address)
{
	if (MsgCache::SendUTXOsMsg != nullptr)
		MsgCache::SendUTXOsMsg = nullptr;

	if (!NetClient::SendMsgRandom(GetUTXOsMsg()))
	{
		LOG_ERROR("No connection to ask UTXO set");

		return {};
	}

	const auto start = Utils::GetUnixTimestamp();
	while (MsgCache::SendUTXOsMsg == nullptr)
	{
		if (Utils::GetUnixTimestamp() - start > MsgCache::MAX_MSG_AWAIT_TIME_IN_SECS)
		{
			LOG_ERROR("Timeout on GetUTXOsMsg");

			return {};
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	std::vector<std::shared_ptr<UTXO>> utxos;
	for (const auto& v : MsgCache::SendUTXOsMsg->UTXO_Map | std::views::values)
	{
		if (v->TxOut->ToAddress == address)
		{
			utxos.push_back(v);
		}
	}
	return utxos;
}
