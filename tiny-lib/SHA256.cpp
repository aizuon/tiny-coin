#include "pch.hpp"
#include "SHA256.hpp"

#include <openssl/sha.h>

EVP_MD* SHA256::md = nullptr;

std::vector<uint8_t> SHA256::HashBinary(const std::vector<uint8_t>& buffer)
{
	std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);

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
	unsigned int md_len = 0;
	if (!EVP_DigestFinal_ex(mdCtx, hash.data(), &md_len))
	{
		EVP_MD_CTX_free(mdCtx);

		return {};
	}
	EVP_MD_CTX_free(mdCtx);

	return hash;
}

std::vector<uint8_t> SHA256::DoubleHashBinary(const std::vector<uint8_t>& buffer)
{
	return HashBinary(HashBinary(buffer));
}
