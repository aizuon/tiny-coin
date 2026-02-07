#pragma once
#include <cstdint>
#include <vector>

class HMACSHA512
{
public:
    static std::vector<uint8_t> hash(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data);
};
