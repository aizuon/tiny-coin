#pragma once
#include <cstdint>
#include <utility>
#include <vector>

#include <openssl/evp.h>
#include <openssl/param_build.h>

class ECDSA
{
public:
	static std::pair<std::vector<uint8_t>, std::vector<uint8_t>> generate();
	static std::vector<uint8_t> get_pub_key_from_priv_key(const std::vector<uint8_t>& priv_key);

	static std::vector<uint8_t> sign_msg(const std::vector<uint8_t>& msg, const std::vector<uint8_t>& priv_key);
	static bool verify_sig(const std::vector<uint8_t>& sig, const std::vector<uint8_t>& msg,
		const std::vector<uint8_t>& pub_key);

private:
	static OSSL_PARAM_BLD* create_param_build();
	static BIGNUM* add_priv_key_param(OSSL_PARAM_BLD* param_bld, const std::vector<uint8_t>& priv_key);
	static bool add_pub_key_param(OSSL_PARAM_BLD* param_bld, const std::vector<uint8_t>& pub_key);
	static EVP_PKEY* create_key(const std::vector<uint8_t>& key, bool priv = false);
	static std::vector<uint8_t> get_pub_key(EVP_PKEY* ec_key);
};
