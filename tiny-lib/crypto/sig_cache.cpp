#include "crypto/sig_cache.hpp"

#include "crypto/sha256.hpp"
#include "util/utils.hpp"

std::unordered_set<std::string> SigCache::cache_;
std::mutex SigCache::mutex_;

std::string SigCache::make_key(const std::vector<uint8_t>& sig, const std::vector<uint8_t>& msg,
    const std::vector<uint8_t>& pub_key)
{
    std::vector<uint8_t> preimage;
    preimage.reserve(sig.size() + msg.size() + pub_key.size());
    preimage.insert(preimage.end(), sig.begin(), sig.end());
    preimage.insert(preimage.end(), msg.begin(), msg.end());
    preimage.insert(preimage.end(), pub_key.begin(), pub_key.end());

    return Utils::byte_array_to_hex_string(SHA256::hash_binary(preimage));
}

bool SigCache::contains(const std::vector<uint8_t>& sig, const std::vector<uint8_t>& msg,
    const std::vector<uint8_t>& pub_key)
{
    const auto key = make_key(sig, msg, pub_key);

    std::scoped_lock lock(mutex_);
    return cache_.count(key) > 0;
}

void SigCache::add(const std::vector<uint8_t>& sig, const std::vector<uint8_t>& msg,
    const std::vector<uint8_t>& pub_key)
{
    const auto key = make_key(sig, msg, pub_key);

    std::scoped_lock lock(mutex_);

    if (cache_.size() >= MAX_ENTRIES)
        cache_.clear();

    cache_.insert(key);
}

void SigCache::clear()
{
    std::scoped_lock lock(mutex_);
    cache_.clear();
}
