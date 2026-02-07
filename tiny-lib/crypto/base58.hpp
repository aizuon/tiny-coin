#pragma once
#include <cstdint>
#include <string>
#include <vector>

class Base58
{
public:
	static std::string encode(const std::vector<uint8_t>& buffer);

private:
	static constexpr char table[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
};
