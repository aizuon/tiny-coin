#include "wallet/hd_wallet.hpp"

#include <fstream>
#include <stdexcept>

#include <openssl/rand.h>

#include "crypto/ecdsa.hpp"
#include "util/log.hpp"
#include "wallet/wallet.hpp"

HDWallet::HDWallet(HDWallet&& other) noexcept
    : seed_(std::move(other.seed_))
    , master_key_(std::move(other.master_key_))
    , account_key_(std::move(other.account_key_))
    , next_external_index_(other.next_external_index_)
    , next_internal_index_(other.next_internal_index_)
    , address_cache_(std::move(other.address_cache_))
{}

HDWallet& HDWallet::operator=(HDWallet&& other) noexcept
{
    if (this != &other)
    {
        seed_ = std::move(other.seed_);
        master_key_ = std::move(other.master_key_);
        account_key_ = std::move(other.account_key_);
        next_external_index_ = other.next_external_index_;
        next_internal_index_ = other.next_internal_index_;
        address_cache_ = std::move(other.address_cache_);
    }
    return *this;
}

HDWallet HDWallet::create()
{
    std::vector<uint8_t> seed(SEED_SIZE);
    if (RAND_bytes(seed.data(), SEED_SIZE) != 1)
        throw std::runtime_error("Failed to generate random seed");

    return from_seed(seed);
}

HDWallet HDWallet::from_seed(const std::vector<uint8_t>& seed)
{
    HDWallet wallet;
    wallet.seed_ = seed;
    wallet.init_from_seed();
    return wallet;
}

HDWallet HDWallet::load(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in.good())
        throw std::runtime_error("Cannot open HD wallet file: " + path);

    uint8_t seed_len = 0;
    in.read(reinterpret_cast<char*>(&seed_len), 1);
    if (seed_len == 0 || seed_len > 64)
        throw std::runtime_error("Invalid seed length in HD wallet file");

    std::vector<uint8_t> seed(seed_len);
    in.read(reinterpret_cast<char*>(seed.data()), seed_len);

    uint32_t ext_index = 0;
    uint32_t int_index = 0;
    in.read(reinterpret_cast<char*>(&ext_index), sizeof(ext_index));
    in.read(reinterpret_cast<char*>(&int_index), sizeof(int_index));

    HDWallet wallet;
    wallet.seed_ = seed;
    wallet.next_external_index_ = ext_index;
    wallet.next_internal_index_ = int_index;
    wallet.init_from_seed();

    return wallet;
}

void HDWallet::save(const std::string& path) const
{
    std::ofstream out(path, std::ios::binary);
    if (!out.good())
        throw std::runtime_error("Cannot write HD wallet file: " + path);

    const auto seed_len = static_cast<uint8_t>(seed_.size());
    out.write(reinterpret_cast<const char*>(&seed_len), 1);
    out.write(reinterpret_cast<const char*>(seed_.data()), seed_len);
    out.write(reinterpret_cast<const char*>(&next_external_index_), sizeof(next_external_index_));
    out.write(reinterpret_cast<const char*>(&next_internal_index_), sizeof(next_internal_index_));
    out.flush();
}

void HDWallet::init_from_seed()
{
    master_key_ = HDKeyChain::from_seed(seed_);

    const auto purpose_key = HDKeyChain::derive_child(master_key_,
        BIP44_PURPOSE | HDKeyChain::HARDENED_BIT);
    const auto coin_key = HDKeyChain::derive_child(purpose_key,
        COIN_TYPE | HDKeyChain::HARDENED_BIT);
    account_key_ = HDKeyChain::derive_child(coin_key,
        DEFAULT_ACCOUNT | HDKeyChain::HARDENED_BIT);

    for (uint32_t i = 0; i < next_external_index_ + LOOKAHEAD; i++)
        derive_and_cache(EXTERNAL_CHAIN, i);
    for (uint32_t i = 0; i < next_internal_index_ + LOOKAHEAD; i++)
        derive_and_cache(INTERNAL_CHAIN, i);
}

void HDWallet::derive_and_cache(uint32_t chain, uint32_t index)
{
    const auto chain_key = HDKeyChain::derive_child(account_key_, chain);
    const auto addr_key = HDKeyChain::derive_child(chain_key, index);

    const auto pub_key = addr_key.get_pub_key();
    const auto address = Wallet::pub_key_to_address(pub_key);

    CachedKey cached;
    cached.priv_key = addr_key.key;
    cached.pub_key = pub_key;
    cached.address = address;
    cached.chain = chain;
    cached.index = index;

    address_cache_[address] = std::move(cached);
}

void HDWallet::fill_lookahead()
{
    for (uint32_t i = 0; i < next_external_index_ + LOOKAHEAD; i++)
    {
        const auto chain_key = HDKeyChain::derive_child(account_key_, EXTERNAL_CHAIN);
        const auto addr_key = HDKeyChain::derive_child(chain_key, i);
        const auto pub_key = addr_key.get_pub_key();
        const auto address = Wallet::pub_key_to_address(pub_key);
        if (!address_cache_.contains(address))
            derive_and_cache(EXTERNAL_CHAIN, i);
    }
    for (uint32_t i = 0; i < next_internal_index_ + LOOKAHEAD; i++)
    {
        const auto chain_key = HDKeyChain::derive_child(account_key_, INTERNAL_CHAIN);
        const auto addr_key = HDKeyChain::derive_child(chain_key, i);
        const auto pub_key = addr_key.get_pub_key();
        const auto address = Wallet::pub_key_to_address(pub_key);
        if (!address_cache_.contains(address))
            derive_and_cache(INTERNAL_CHAIN, i);
    }
}

std::string HDWallet::get_new_address()
{
    std::scoped_lock lock(mutex_);

    const auto chain_key = HDKeyChain::derive_child(account_key_, EXTERNAL_CHAIN);
    const auto addr_key = HDKeyChain::derive_child(chain_key, next_external_index_);
    const auto pub_key = addr_key.get_pub_key();
    const auto address = Wallet::pub_key_to_address(pub_key);

    if (!address_cache_.contains(address))
        derive_and_cache(EXTERNAL_CHAIN, next_external_index_);

    next_external_index_++;
    fill_lookahead();

    LOG_INFO("HD wallet: new receiving address {} (m/44'/0'/0'/0/{})",
        address, next_external_index_ - 1);

    return address;
}

std::string HDWallet::get_change_address()
{
    std::scoped_lock lock(mutex_);

    const auto chain_key = HDKeyChain::derive_child(account_key_, INTERNAL_CHAIN);
    const auto addr_key = HDKeyChain::derive_child(chain_key, next_internal_index_);
    const auto pub_key = addr_key.get_pub_key();
    const auto address = Wallet::pub_key_to_address(pub_key);

    if (!address_cache_.contains(address))
        derive_and_cache(INTERNAL_CHAIN, next_internal_index_);

    next_internal_index_++;
    fill_lookahead();

    LOG_INFO("HD wallet: new change address {} (m/44'/0'/0'/1/{})",
        address, next_internal_index_ - 1);

    return address;
}

std::vector<std::string> HDWallet::get_all_addresses() const
{
    std::scoped_lock lock(mutex_);

    std::vector<std::string> addresses;
    addresses.reserve(address_cache_.size());
    for (const auto& [addr, _] : address_cache_)
        addresses.push_back(addr);
    return addresses;
}

bool HDWallet::get_keys_for_address(const std::string& address,
    std::vector<uint8_t>& priv_key, std::vector<uint8_t>& pub_key) const
{
    std::scoped_lock lock(mutex_);

    const auto it = address_cache_.find(address);
    if (it == address_cache_.end())
        return false;

    priv_key = it->second.priv_key;
    pub_key = it->second.pub_key;
    return true;
}

bool HDWallet::owns_address(const std::string& address) const
{
    std::scoped_lock lock(mutex_);
    return address_cache_.contains(address);
}

std::string HDWallet::get_primary_address() const
{
    std::scoped_lock lock(mutex_);

    const auto chain_key = HDKeyChain::derive_child(account_key_, EXTERNAL_CHAIN);
    const auto addr_key = HDKeyChain::derive_child(chain_key, 0);
    return Wallet::pub_key_to_address(addr_key.get_pub_key());
}

std::vector<uint8_t> HDWallet::get_primary_priv_key() const
{
    std::scoped_lock lock(mutex_);

    const auto chain_key = HDKeyChain::derive_child(account_key_, EXTERNAL_CHAIN);
    const auto addr_key = HDKeyChain::derive_child(chain_key, 0);
    return addr_key.key;
}

std::vector<uint8_t> HDWallet::get_primary_pub_key() const
{
    std::scoped_lock lock(mutex_);

    const auto chain_key = HDKeyChain::derive_child(account_key_, EXTERNAL_CHAIN);
    const auto addr_key = HDKeyChain::derive_child(chain_key, 0);
    return addr_key.get_pub_key();
}
