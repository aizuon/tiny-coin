#include "pch.hpp"

#include <algorithm>
#include <openssl/bn.h>

#include "Base58.hpp"
#include "Utils.hpp"

std::string Base58::Encode(const std::vector<uint8_t>& buffer)
{
    std::string result;
    BN_CTX* bnctx = BN_CTX_new();
    BIGNUM* bn = BN_new();
    BIGNUM* bn00 = BN_new();
    BIGNUM* bn58 = BN_new();
    BIGNUM* dv = BN_new();
    BIGNUM* rem = BN_new();

    auto hexString = Utils::ByteArrayToHexString(buffer);

    BN_hex2bn(&bn, hexString.c_str());
    BN_hex2bn(&bn58, "3a");
    BN_hex2bn(&bn00, "00");
    while (BN_cmp(bn, bn00) > 0)
    {
        BN_div(dv, rem, bn, bn58, bnctx);
        BN_copy(bn, dv);
        char base58char = Table[BN_get_word(rem)];
        result += base58char;
    }

    BN_free(rem);
    BN_free(dv);
    BN_free(bn58);
    BN_free(bn00);
    BN_free(bn);
    BN_CTX_free(bnctx);

    std::reverse(result.begin(), result.end());

    return result;
}
