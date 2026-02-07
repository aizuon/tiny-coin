#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct ExtendedKey
{
    std::vector<uint8_t> key;
    std::vector<uint8_t> chain_code;
    uint8_t depth = 0;
    uint32_t child_number = 0;
    std::vector<uint8_t> parent_fingerprint;

    std::vector<uint8_t> get_pub_key() const;
    std::vector<uint8_t> get_fingerprint() const;
};

class HDKeyChain
{
public:
    static constexpr uint32_t HARDENED_BIT = 0x80000000;

    static ExtendedKey from_seed(const std::vector<uint8_t>& seed);
    static ExtendedKey derive_child(const ExtendedKey& parent, uint32_t index);
    static ExtendedKey derive_path(const ExtendedKey& root, const std::string& path);

private:
    static std::vector<uint8_t> serialize_uint32_be(uint32_t val);
    static std::vector<uint8_t> pad_key_32(const std::vector<uint8_t>& key);
};
