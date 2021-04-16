#include "pch.hpp"

#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/ec.h>

#include "ECDSA.hpp"

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> ECDSA::Generate()
{
    EC_KEY* ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!EC_KEY_generate_key(ec_key))
        return { std::vector<uint8_t>(), std::vector<uint8_t>() };

    const EC_GROUP* ec_group = EC_KEY_get0_group(ec_key);

    const BIGNUM* priv_key = EC_KEY_get0_private_key(ec_key);
    const EC_POINT* pub_key = EC_KEY_get0_public_key(ec_key);

    std::vector<uint8_t> priv_key_buffer(BN_num_bytes(priv_key));
    BN_bn2bin(priv_key, priv_key_buffer.data());

    BN_CTX* bn_ctx = BN_CTX_new();
    BIGNUM* pub_key_bn = BN_new();
    EC_POINT_point2bn(ec_group, pub_key, POINT_CONVERSION_UNCOMPRESSED, pub_key_bn, bn_ctx);
    std::vector<uint8_t> pub_key_buffer(BN_num_bytes(pub_key_bn));
    BN_bn2bin(pub_key_bn, pub_key_buffer.data());
    BN_free(pub_key_bn);
    BN_CTX_free(bn_ctx);

    EC_KEY_free(ec_key);

    return { priv_key_buffer, pub_key_buffer };
}
