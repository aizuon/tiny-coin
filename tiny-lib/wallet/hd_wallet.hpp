#pragma once
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "wallet/hd_key_chain.hpp"

class HDWallet
{
public:
    static constexpr uint32_t BIP44_PURPOSE = 44;
    static constexpr uint32_t COIN_TYPE = 0;
    static constexpr uint32_t DEFAULT_ACCOUNT = 0;
    static constexpr uint32_t EXTERNAL_CHAIN = 0;
    static constexpr uint32_t INTERNAL_CHAIN = 1;
    static constexpr uint32_t LOOKAHEAD = 20;
    static constexpr uint8_t SEED_SIZE = 32;

    static HDWallet create();
    static HDWallet from_seed(const std::vector<uint8_t>& seed);
    static HDWallet load(const std::string& path);
    void save(const std::string& path) const;

    HDWallet(HDWallet&& other) noexcept;
    HDWallet& operator=(HDWallet&& other) noexcept;
    HDWallet(const HDWallet&) = delete;
    HDWallet& operator=(const HDWallet&) = delete;

    std::string get_new_address();
    std::string get_change_address();
    std::vector<std::string> get_all_addresses() const;

    bool get_keys_for_address(const std::string& address,
        std::vector<uint8_t>& priv_key, std::vector<uint8_t>& pub_key) const;

    bool owns_address(const std::string& address) const;

    const std::vector<uint8_t>& get_seed() const { return seed_; }
    uint32_t get_external_index() const { return next_external_index_; }
    uint32_t get_internal_index() const { return next_internal_index_; }

    std::string get_primary_address() const;
    std::vector<uint8_t> get_primary_priv_key() const;
    std::vector<uint8_t> get_primary_pub_key() const;

private:
    HDWallet() = default;

    void init_from_seed();
    void derive_and_cache(uint32_t chain, uint32_t index);
    void fill_lookahead();

    std::vector<uint8_t> seed_;
    ExtendedKey master_key_;
    ExtendedKey account_key_;

    uint32_t next_external_index_ = 0;
    uint32_t next_internal_index_ = 0;

    mutable std::mutex mutex_;

    struct CachedKey
    {
        std::vector<uint8_t> priv_key;
        std::vector<uint8_t> pub_key;
        std::string address;
        uint32_t chain;
        uint32_t index;
    };
    std::map<std::string, CachedKey> address_cache_;
};
