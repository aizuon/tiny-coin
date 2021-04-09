#pragma once
#include <cstdint>

class NetParams
{
public:
	static constexpr uint64_t MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES = 1e6;

	static constexpr uint8_t COINBASE_MATURITY = 2;

	static constexpr uint64_t MAX_FUTURE_BLOCK_TIME = (60 * 60 * 2);

	static constexpr uint64_t COIN = 1e6;
	static constexpr uint64_t TOTAL_COINS = 21e6;
	static constexpr uint64_t MAX_MONEY = COIN * TOTAL_COINS;

	static constexpr uint64_t TIME_BETWEEN_BLOCKS_IN_SECS_TARGET = 1 * 60;
	static constexpr uint64_t DIFFICULTY_PERIOD_IN_SECS_TARGET = (60 * 60 * 10);
	static constexpr double DIFFICULTY_PERIOD_IN_BLOCKS = DIFFICULTY_PERIOD_IN_SECS_TARGET / TIME_BETWEEN_BLOCKS_IN_SECS_TARGET;
	static constexpr uint8_t INITIAL_DIFFICULTY_BITS = 24;
	static constexpr uint64_t HALVE_SUBSIDY_AFTER_BLOCKS_NUM = 21e3;
};