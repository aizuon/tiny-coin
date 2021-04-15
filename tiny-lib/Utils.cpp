#include "pch.hpp"

#include <chrono>
#include <algorithm>
#include <boost/algorithm/hex.hpp>

#include "Utils.hpp"

std::string Utils::ByteArrayToHexString(const std::vector<uint8_t>& vec)
{
    std::string hash;

    boost::algorithm::hex_lower(vec.begin(), vec.end(), back_inserter(hash));

    return hash;
}

std::vector<uint8_t> Utils::HexStringToByteArray(const std::string& str)
{
    std::string hash = boost::algorithm::unhex(str);

    std::vector<uint8_t> vec(hash.size());

    std::copy(hash.begin(), hash.end(), vec.data());

    return vec;
}

int64_t Utils::GetUnixTimestamp()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}
