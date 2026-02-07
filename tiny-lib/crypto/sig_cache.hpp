#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

class SigCache
{
public:
    static constexpr uint32_t MAX_ENTRIES = 100000;

    static bool contains(const std::vector<uint8_t>& sig, const std::vector<uint8_t>& msg,
        const std::vector<uint8_t>& pub_key);

    static void add(const std::vector<uint8_t>& sig, const std::vector<uint8_t>& msg,
        const std::vector<uint8_t>& pub_key);

    static void clear();

private:
    static std::string make_key(const std::vector<uint8_t>& sig, const std::vector<uint8_t>& msg,
        const std::vector<uint8_t>& pub_key);

    static std::unordered_set<std::string> cache_;
    static std::mutex mutex_;
};
