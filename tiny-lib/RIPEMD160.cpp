#include "pch.hpp"
#include "RIPEMD160.hpp"

#include <openssl/ripemd.h>

EVP_MD* RIPEMD160::md = nullptr;

std::vector<uint8_t> RIPEMD160::HashBinary(const std::vector<uint8_t>& buffer)
{
	std::vector<uint8_t> hash(RIPEMD160_DIGEST_LENGTH);

	EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
	if (md_ctx == nullptr)
	{
		EVP_MD_CTX_free(md_ctx);

		return {};
	}
	if (!EVP_DigestInit_ex(md_ctx, md, nullptr))
	{
		EVP_MD_CTX_free(md_ctx);

		return {};
	}
	if (!EVP_DigestUpdate(md_ctx, buffer.data(), buffer.size()))
	{
		EVP_MD_CTX_free(md_ctx);

		return {};
	}
	if (!EVP_DigestFinal_ex(md_ctx, hash.data(), nullptr))
	{
		EVP_MD_CTX_free(md_ctx);

		return {};
	}
	EVP_MD_CTX_free(md_ctx);

	return hash;
}
