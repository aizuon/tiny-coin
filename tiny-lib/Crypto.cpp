#include "pch.hpp"
#include "Crypto.hpp"

#include <exception>
#include <openssl/evp.h>

#include "SHA256.hpp"
#include "RIPEMD160.hpp"

OSSL_PROVIDER* Crypto::pDefault = nullptr;
OSSL_PROVIDER* Crypto::pLegacy = nullptr;

void Crypto::Init()
{
	pDefault = OSSL_PROVIDER_load(nullptr, "default");
	if (pDefault == nullptr)
		throw std::exception("Failed to load default OpenSSL provider");
	pLegacy = OSSL_PROVIDER_load(nullptr, "legacy");
	if (pLegacy == nullptr)
		throw std::exception("Failed to load legacy OpenSSL provider");

	SHA256::md = EVP_MD_fetch(nullptr, "SHA256", nullptr);
	if (SHA256::md == nullptr)
		throw std::exception("Failed to load SHA256");
	RIPEMD160::md = EVP_MD_fetch(nullptr, "RIPEMD160", nullptr);
	if (RIPEMD160::md == nullptr)
		throw std::exception("Failed to load RIPEMD160");
}

void Crypto::CleanUp()
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

	if (pLegacy != nullptr)
	{
		OSSL_PROVIDER_unload(pLegacy);
		pLegacy = nullptr;
	}
	if (pDefault != nullptr)
	{
		OSSL_PROVIDER_unload(pDefault);
		pDefault = nullptr;
	}
}
