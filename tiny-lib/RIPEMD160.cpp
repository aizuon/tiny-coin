#include "pch.hpp"
#include "RIPEMD160.hpp"

#include <openssl/ripemd.h>

EVP_MD* RIPEMD160::md = nullptr;

std::vector<uint8_t> RIPEMD160::HashBinary(const std::vector<uint8_t>& buffer)
{
	std::vector<uint8_t> hash(RIPEMD160_DIGEST_LENGTH);

	EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();
	if (mdCtx == nullptr)
	{
		EVP_MD_CTX_free(mdCtx);

		return {};
	}
	if (!EVP_DigestInit_ex(mdCtx, md, nullptr))
	{
		EVP_MD_CTX_free(mdCtx);

		return {};
	}
	if (!EVP_DigestUpdate(mdCtx, buffer.data(), buffer.size()))
	{
		EVP_MD_CTX_free(mdCtx);

		return {};
	}
	if (!EVP_DigestFinal_ex(mdCtx, hash.data(), nullptr))
	{
		EVP_MD_CTX_free(mdCtx);

		return {};
	}
	EVP_MD_CTX_free(mdCtx);

	return hash;
}
