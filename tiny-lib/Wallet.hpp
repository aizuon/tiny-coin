#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "Enums.hpp"
#include "Tx.hpp"
#include "TxIn.hpp"
#include "TxOut.hpp"
#include "TxOutPoint.hpp"

#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")

class UnspentTxOut;

class Wallet
{
public:
	static std::string PubKeyToAddress(const std::vector<uint8_t>& pubKey);

	static std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string>
	InitWallet(const std::string& walletPath);
	static std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> InitWallet();

	static std::shared_ptr<TxIn> BuildTxIn(const std::vector<uint8_t>& privKey,
	                                      const std::shared_ptr<TxOutPoint>& txOutPoint,
	                                      const std::shared_ptr<TxOut>& txOut);
	static std::shared_ptr<Tx> SendValue_Miner(uint64_t value, const std::string& address,
	                                           const std::vector<uint8_t>& privKey);
	static std::shared_ptr<Tx> SendValue(uint64_t value, const std::string& address,
	                                     const std::vector<uint8_t>& privKey);

	struct TxStatusResponse
	{
		TxStatus Status = TxStatus::NotFound;
		std::string BlockId;
		int64_t BlockHeight = -1;
	};

	static TxStatusResponse GetTxStatus_Miner(const std::string& txId);
	static TxStatusResponse GetTxStatus(const std::string& txId);
	static void PrintTxStatus(const std::string& txId);

	static uint64_t GetBalance_Miner(const std::string& address);
	static uint64_t GetBalance(const std::string& address);
	static void PrintBalance(const std::string& address);

private:
	static constexpr char PubKeyHashVersion = '1';
	static constexpr char DefaultWalletPath[] = "wallet.dat";
	static std::string WalletPath;

	static std::shared_ptr<Tx> BuildTxFromUTXOs(std::vector<std::shared_ptr<UnspentTxOut>>& utxos, uint64_t value,
	                                            const std::string& address,
	                                            const std::vector<uint8_t>& privKey);

	static std::shared_ptr<Tx> BuildTx_Miner(uint64_t value, const std::string& address,
		const std::vector<uint8_t>& privKey);
	static std::shared_ptr<Tx> BuildTx(uint64_t value, const std::string& address, const std::vector<uint8_t>& privKey);

	static std::vector<std::shared_ptr<UnspentTxOut>> FindUTXOsForAddress_Miner(const std::string& address);
	static std::vector<std::shared_ptr<UnspentTxOut>> FindUTXOsForAddress(const std::string& address);
};
