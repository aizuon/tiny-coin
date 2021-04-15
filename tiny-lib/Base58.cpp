#include "pch.hpp"

#include <openssl/bn.h>

#include "Base58.hpp"
#include "Utils.hpp"

std::string Base58::Encode(const std::vector<uint8_t>& buffer)
{
    std::string result;
    BN_CTX* bnctx = BN_CTX_new();
    BIGNUM* bn = BN_new();
    BIGNUM* bn0 = BN_new();
    BIGNUM* bn58 = BN_new();
    BIGNUM* dv = BN_new();
    BIGNUM* rem = BN_new();

    auto hexString = Utils::ByteArrayToHexString(buffer);

    BN_hex2bn(&bn, hexString.c_str());
    BN_hex2bn(&bn58, "3a");
    BN_hex2bn(&bn0, "00");
    while (BN_cmp(bn, bn0) > 0)
    {
        BN_div(dv, rem, bn, bn58, bnctx);
        BN_copy(bn, dv);
        char base58char = Table[BN_get_word(rem)];
        result += base58char;
    }

    BN_CTX_free(bnctx);
    BN_free(bn);
    BN_free(bn0);
    BN_free(bn58);
    BN_free(dv);
    BN_free(rem);

    std::string::iterator pbegin = result.begin();
    std::string::iterator pend = result.end();
    while (pbegin < pend)
    {
        char c = *pbegin;
        *(pbegin++) = *(--pend);
        *pend = c;
    }
    return result;
}
