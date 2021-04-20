#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <tuple>

#include "TxIn.hpp"
#include "TxOut.hpp"
#include "TxOutPoint.hpp"
#include "UnspentTxOut.hpp"

class Wallet
{
public:
	static std::string PubKeyToAddress(const std::vector<uint8_t>& pubKey);

	static std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> InitWallet(const std::string& walletPath);
	static std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> InitWallet();

	static std::shared_ptr<TxIn> MakeTxIn(const std::vector<uint8_t>& privKey, const std::shared_ptr<TxOutPoint>& txOutPoint, const std::shared_ptr<TxOut>& txOut);
	static void SendValue(uint64_t value, const std::string& address, const std::vector<uint8_t>& privKey);

	static void PrintTxStatus(const std::string& txId);

	static uint64_t GetBalance(const std::string& address);

private:
	static constexpr char PubKeyHashVersion = '1';
	static constexpr char DefaultWalletPath[] = "wallet.dat";
	static std::string WalletPath;

	static std::vector<std::shared_ptr<UnspentTxOut>> FindUTXOsForAddress(const std::string& address);
};