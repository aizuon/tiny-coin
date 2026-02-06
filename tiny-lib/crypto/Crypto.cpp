#include "crypto/crypto.hpp"

#include <stdexcept>
#include <openssl/evp.h>

#include "crypto/sha256.hpp"
#include "crypto/ripemd160.hpp"

OSSL_PROVIDER* Crypto::p_default = nullptr;
OSSL_PROVIDER* Crypto::p_legacy = nullptr;

void Crypto::init()
{
	p_default = OSSL_PROVIDER_load(nullptr, "default");
	if (p_default == nullptr)
		throw std::runtime_error("Failed to load default OpenSSL provider");
	p_legacy = OSSL_PROVIDER_load(nullptr, "legacy");
	if (p_legacy == nullptr)
		throw std::runtime_error("Failed to load legacy OpenSSL provider");

	SHA256::md = EVP_MD_fetch(nullptr, "SHA256", nullptr);
	if (SHA256::md == nullptr)
		throw std::runtime_error("Failed to load SHA256");
	RIPEMD160::md = EVP_MD_fetch(nullptr, "RIPEMD160", nullptr);
	if (RIPEMD160::md == nullptr)
		throw std::runtime_error("Failed to load RIPEMD160");
}

void Crypto::cleanup()
{
	if (RIPEMD160::md != nullptr)
	{
		EVP_MD_free(RIPEMD160::md);
		RIPEMD160::md = nullptr;
	}
	if (SHA256::md != nullptr)
	{
		EVP_MD_free(SHA256::md);
		SHA256::md = nullptr;
	}

	if (p_legacy != nullptr)
	{
		OSSL_PROVIDER_unload(p_legacy);
		p_legacy = nullptr;
	}
	if (p_default != nullptr)
	{
		OSSL_PROVIDER_unload(p_default);
		p_default = nullptr;
	}
}
