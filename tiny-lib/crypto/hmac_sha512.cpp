#include "crypto/hmac_sha512.hpp"

#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/params.h>

std::vector<uint8_t> HMACSHA512::hash(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data)
{
    auto* mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
    if (mac == nullptr)
        return {};

    auto* ctx = EVP_MAC_CTX_new(mac);
    if (ctx == nullptr)
    {
        EVP_MAC_free(mac);
        return {};
    }

    char digest_name[] = "SHA512";
    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, digest_name, 0),
        OSSL_PARAM_construct_end()
    };

    if (!EVP_MAC_init(ctx, key.data(), key.size(), params))
    {
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return {};
    }

    if (!EVP_MAC_update(ctx, data.data(), data.size()))
    {
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return {};
    }

    std::vector<uint8_t> result(64);
    size_t out_len = 64;
    if (!EVP_MAC_final(ctx, result.data(), &out_len, result.size()))
    {
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return {};
    }

    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);

    return result;
}
