#pragma once
#include <cstdint>
#include <utility>
#include <vector>

#include <openssl/ec.h>

#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")

class ECDSA
{
public:
	static std::pair<std::vector<uint8_t>, std::vector<uint8_t>> Generate();
	static std::vector<uint8_t> GetPubKeyFromPrivKey(const std::vector<uint8_t>& privKey);

	static std::vector<uint8_t> SignMsg(const std::vector<uint8_t>& msg, const std::vector<uint8_t>& privKey);
	static bool VerifySig(const std::vector<uint8_t>& sig, const std::vector<uint8_t>& msg,
	                      const std::vector<uint8_t>& pubKey);

private:
	static bool ImportPrivKey(EC_KEY* ec_key, const std::vector<uint8_t>& privKey);
	static std::vector<uint8_t> GetPubKeyFromPrivKey(EC_KEY* ec_key, const EC_GROUP* ec_group);
};
