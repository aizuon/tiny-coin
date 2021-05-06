#pragma once
#include <cstdint>

enum class TxStatus : uint8_t
{
	Mempool,
	Mined,
	NotFound
};
