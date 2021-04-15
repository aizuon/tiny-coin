#pragma once
#include <cstdint>
#include <vector>
#include <string>

class Wallet
{
public:
	static std::string PubKeyToAddress(const std::vector<uint8_t>& pubKey);

private:
	static constexpr char PubKeyHashVersion = '1';
};