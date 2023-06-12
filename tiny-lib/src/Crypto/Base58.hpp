#pragma once
#include <cstdint>
#include <string>
#include <vector>

class Base58
{
public:
	static std::string Encode(const std::vector<uint8_t>& buffer);

private:
	static constexpr char Table[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
};
