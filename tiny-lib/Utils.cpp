#include "pch.hpp"

#include <chrono>

#include "Utils.hpp"

std::string Utils::ByteArrayToHexString(const std::vector<uint8_t>& vec)
{
    std::string hash;
    hash.resize(vec.size() * 2);

    for (size_t i = 0; i < vec.size(); i++)
        sprintf_s(hash.data() + (i * 2), 2, "%02x", vec[i]);

    return hash;
}

int64_t Utils::GetUnixTimestamp()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}
