#pragma once
#include <cstdint>
#include <vector>
#include <utility>

class ECDSA
{
public:
	static std::pair<std::vector<uint8_t>, std::vector<uint8_t>> Generate();
};