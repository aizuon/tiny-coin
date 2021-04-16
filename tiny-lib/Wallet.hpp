#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <tuple>

class Wallet
{
public:
	static std::string PubKeyToAddress(const std::vector<uint8_t>& pubKey);

	static std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> InitWallet();

private:
	static constexpr char PubKeyHashVersion = '1';
	static constexpr char WalletPath[] = "wallet.dat";
};