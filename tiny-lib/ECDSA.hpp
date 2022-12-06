#pragma once
#include <cstdint>
#include <utility>
#include <vector>
#include <utility>

#include <openssl/evp.h>
#include <openssl/param_build.h>

#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")

class ECDSA
{
public:
	static std::pair<std::vector<uint8_t>, std::vector<uint8_t>> Generate();
	static std::vector<uint8_t> GetPubKeyFromPrivKey(const std::vector<uint8_t>& priv_key);

	static std::vector<uint8_t> SignMsg(const std::vector<uint8_t>& msg, const std::vector<uint8_t>& priv_key);
	static bool VerifySig(const std::vector<uint8_t>& sig, const std::vector<uint8_t>& msg,
	                      const std::vector<uint8_t>& pub_key);

private:
	static OSSL_PARAM_BLD* CreateParamBuild();
	static bool AddPrivKeyParam(OSSL_PARAM_BLD* param_bld, const std::vector<uint8_t>& priv_key);
	static bool AddPubKeyParam(OSSL_PARAM_BLD* param_bld, const std::vector<uint8_t>& pub_key);
	static EVP_PKEY* CreateKey(const std::vector<uint8_t>& key, bool priv = false);
	static std::vector<uint8_t> GetPubKey(EVP_PKEY* ec_key);
};
