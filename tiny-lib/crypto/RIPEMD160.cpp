#include "crypto/ripemd160.hpp"

#include <memory>

#include <openssl/ripemd.h>

EVP_MD* RIPEMD160::md = nullptr;

std::vector<uint8_t> RIPEMD160::hash_binary(const std::vector<uint8_t>& buffer)
{
	std::vector<uint8_t> hash(RIPEMD160_DIGEST_LENGTH);

	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> md_ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
	if (!md_ctx)
		return {};
	if (!EVP_DigestInit_ex(md_ctx.get(), md, nullptr))
		return {};
	if (!EVP_DigestUpdate(md_ctx.get(), buffer.data(), buffer.size()))
		return {};
	if (!EVP_DigestFinal_ex(md_ctx.get(), hash.data(), nullptr))
		return {};

	return hash;
}
