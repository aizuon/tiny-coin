#include "wallet/hd_key_chain.hpp"

#include <sstream>
#include <stdexcept>
#include <vector>

#include <openssl/bn.h>
#include <openssl/ec.h>

#include "crypto/ecdsa.hpp"
#include "crypto/hmac_sha512.hpp"
#include "crypto/ripemd160.hpp"
#include "crypto/sha256.hpp"
#include "util/utils.hpp"

static const char* SECP256K1_ORDER_HEX =
"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141";

std::vector<uint8_t> ExtendedKey::get_pub_key() const
{
    return ECDSA::get_pub_key_from_priv_key(key);
}

std::vector<uint8_t> ExtendedKey::get_fingerprint() const
{
    const auto pub_key = get_pub_key();
    if (pub_key.empty())
        return { 0, 0, 0, 0 };

    const auto sha = SHA256::hash_binary(pub_key);
    const auto ripe = RIPEMD160::hash_binary(sha);

    return { ripe[0], ripe[1], ripe[2], ripe[3] };
}

ExtendedKey HDKeyChain::from_seed(const std::vector<uint8_t>& seed)
{
    const std::string hmac_key_str = "Bitcoin seed";
    const std::vector<uint8_t> hmac_key(hmac_key_str.begin(), hmac_key_str.end());

    const auto I = HMACSHA512::hash(hmac_key, seed);
    if (I.size() != 64)
        throw std::runtime_error("HMAC-SHA512 failed during master key generation");

    ExtendedKey master;
    master.key = pad_key_32(std::vector<uint8_t>(I.begin(), I.begin() + 32));
    master.chain_code = std::vector<uint8_t>(I.begin() + 32, I.end());
    master.depth = 0;
    master.child_number = 0;
    master.parent_fingerprint = { 0, 0, 0, 0 };

    BIGNUM* key_bn = BN_bin2bn(master.key.data(), static_cast<int>(master.key.size()), nullptr);
    BIGNUM* order = nullptr;
    BN_hex2bn(&order, SECP256K1_ORDER_HEX);

    if (BN_is_zero(key_bn) || BN_cmp(key_bn, order) >= 0)
    {
        BN_free(key_bn);
        BN_free(order);
        throw std::runtime_error("Invalid master key derived from seed");
    }

    BN_free(key_bn);
    BN_free(order);

    return master;
}

ExtendedKey HDKeyChain::derive_child(const ExtendedKey& parent, uint32_t index)
{
    std::vector<uint8_t> data;

    if (index >= HARDENED_BIT)
    {
        data.push_back(0x00);
        const auto padded = pad_key_32(parent.key);
        data.insert(data.end(), padded.begin(), padded.end());
    }
    else
    {
        const auto pub_key = parent.get_pub_key();
        if (pub_key.empty())
            throw std::runtime_error("Failed to derive public key for normal child derivation");
        data.insert(data.end(), pub_key.begin(), pub_key.end());
    }

    const auto index_bytes = serialize_uint32_be(index);
    data.insert(data.end(), index_bytes.begin(), index_bytes.end());

    const auto I = HMACSHA512::hash(parent.chain_code, data);
    if (I.size() != 64)
        throw std::runtime_error("HMAC-SHA512 failed during child key derivation");

    const std::vector<uint8_t> IL(I.begin(), I.begin() + 32);
    const std::vector<uint8_t> IR(I.begin() + 32, I.end());

    BIGNUM* il_bn = BN_bin2bn(IL.data(), static_cast<int>(IL.size()), nullptr);
    BIGNUM* kpar_bn = BN_bin2bn(parent.key.data(), static_cast<int>(parent.key.size()), nullptr);
    BIGNUM* order = nullptr;
    BN_hex2bn(&order, SECP256K1_ORDER_HEX);

    if (BN_cmp(il_bn, order) >= 0)
    {
        BN_free(il_bn);
        BN_free(kpar_bn);
        BN_free(order);
        throw std::runtime_error("Derived key IL >= curve order");
    }

    BIGNUM* child_bn = BN_new();
    BN_CTX* bn_ctx = BN_CTX_new();
    BN_mod_add(child_bn, il_bn, kpar_bn, order, bn_ctx);

    if (BN_is_zero(child_bn))
    {
        BN_free(il_bn);
        BN_free(kpar_bn);
        BN_free(order);
        BN_free(child_bn);
        BN_CTX_free(bn_ctx);
        throw std::runtime_error("Derived child key is zero");
    }

    std::vector<uint8_t> child_key_raw(BN_num_bytes(child_bn));
    BN_bn2bin(child_bn, child_key_raw.data());

    BN_free(il_bn);
    BN_free(kpar_bn);
    BN_free(order);
    BN_free(child_bn);
    BN_CTX_free(bn_ctx);

    ExtendedKey child;
    child.key = pad_key_32(child_key_raw);
    child.chain_code = IR;
    child.depth = parent.depth + 1;
    child.child_number = index;
    child.parent_fingerprint = parent.get_fingerprint();

    return child;
}

ExtendedKey HDKeyChain::derive_path(const ExtendedKey& root, const std::string& path)
{
    std::string p = path;
    if (p.empty())
        throw std::runtime_error("Empty derivation path");

    if (p[0] == 'm')
    {
        if (p.size() == 1)
            return root;
        if (p[1] != '/')
            throw std::runtime_error("Invalid derivation path format");
        p = p.substr(2);
    }

    ExtendedKey current = root;
    std::istringstream stream(p);
    std::string component;

    while (std::getline(stream, component, '/'))
    {
        if (component.empty())
            continue;

        bool hardened = false;
        if (component.back() == '\'' || component.back() == 'h' || component.back() == 'H')
        {
            hardened = true;
            component.pop_back();
        }

        uint32_t index;
        try
        {
            index = static_cast<uint32_t>(std::stoul(component));
        }
        catch (const std::exception&)
        {
            throw std::runtime_error("Invalid index in derivation path: " + component);
        }

        if (hardened)
            index |= HARDENED_BIT;

        current = derive_child(current, index);
    }

    return current;
}

std::vector<uint8_t> HDKeyChain::serialize_uint32_be(uint32_t val)
{
    return {
        static_cast<uint8_t>((val >> 24) & 0xFF),
        static_cast<uint8_t>((val >> 16) & 0xFF),
        static_cast<uint8_t>((val >> 8) & 0xFF),
        static_cast<uint8_t>(val & 0xFF)
    };
}

std::vector<uint8_t> HDKeyChain::pad_key_32(const std::vector<uint8_t>& key)
{
    if (key.size() == 32)
        return key;

    if (key.size() > 32)
        return std::vector<uint8_t>(key.end() - 32, key.end());

    std::vector<uint8_t> padded(32, 0);
    std::copy(key.begin(), key.end(), padded.begin() + (32 - key.size()));
    return padded;
}
