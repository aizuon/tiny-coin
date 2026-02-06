#include "crypto/sha256.hpp"

#include <memory>

#include <openssl/sha.h>

EVP_MD* SHA256::md = nullptr;

std::vector<uint8_t> SHA256::hash_binary(const std::vector<uint8_t>& buffer)
{
	std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);

	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> md_ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
	if (!md_ctx)
		return {};
	if (!EVP_DigestInit_ex(md_ctx.get(), md, nullptr))
		return {};
	if (!EVP_DigestUpdate(md_ctx.get(), buffer.data(), buffer.size()))
		return {};
	unsigned int md_len = 0;
	if (!EVP_DigestFinal_ex(md_ctx.get(), hash.data(), &md_len))
		return {};

	return hash;
}

std::vector<uint8_t> SHA256::double_hash_binary(const std::vector<uint8_t>& buffer)
{
	std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);

	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> md_ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
	if (!md_ctx)
		return {};

	if (!EVP_DigestInit_ex(md_ctx.get(), md, nullptr))
		return {};
	if (!EVP_DigestUpdate(md_ctx.get(), buffer.data(), buffer.size()))
		return {};
	unsigned int md_len = 0;
	if (!EVP_DigestFinal_ex(md_ctx.get(), hash.data(), &md_len))
		return {};

	if (!EVP_DigestInit_ex(md_ctx.get(), md, nullptr))
		return {};
	if (!EVP_DigestUpdate(md_ctx.get(), hash.data(), md_len))
		return {};
	if (!EVP_DigestFinal_ex(md_ctx.get(), hash.data(), &md_len))
		return {};

	return hash;
}
