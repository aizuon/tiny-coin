#pragma once
#include <cstdint>
#include <string>

class PoW
{
public:
	static uint8_t GetNextWorkRequired(const std::string& prevBlockHash);
};