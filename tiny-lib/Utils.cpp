#include "pch.hpp"

#include <sstream>
#include <iomanip>
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

std::vector<uint8_t> Utils::StringToByteArray(const std::string& str)
{
    std::vector<uint8_t> vec(str.begin(), str.end());

    return vec;
}

std::string Utils::ByteArrayToHexString_DEBUG(const std::vector<uint8_t>& vec)
{
    std::stringstream ss;
    ss << std::hex;

    for (auto b : vec)
        ss << std::setw(2) << std::setfill('0') << (int)b << " ";

    std::string output = ss.str();
    std::transform(output.begin(), output.end(), output.begin(), std::toupper);

    return output;
}

int64_t Utils::GetUnixTimestamp()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}
