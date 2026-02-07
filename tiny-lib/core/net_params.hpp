#pragma once
#include <cstdint>
#include <string>

class NetParams
{
public:
	static constexpr uint32_t MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES = 1000000;

	static constexpr uint8_t COINBASE_MATURITY = 2;

	static constexpr uint32_t MAX_FUTURE_BLOCK_TIME_IN_SECS = 60 * 60 * 2;

	static constexpr uint64_t COIN = 100000000;
	static constexpr uint64_t TOTAL_COINS = 21000000;
	static constexpr uint64_t MAX_MONEY = COIN * TOTAL_COINS;

	static constexpr uint32_t TIME_BETWEEN_BLOCKS_IN_SECS_TARGET = 60 * 10;
	static constexpr uint32_t DIFFICULTY_PERIOD_IN_SECS_TARGET = 60 * 60 * 24;
	static constexpr uint32_t DIFFICULTY_PERIOD_IN_BLOCKS = DIFFICULTY_PERIOD_IN_SECS_TARGET /
		TIME_BETWEEN_BLOCKS_IN_SECS_TARGET;
	static constexpr uint8_t INITIAL_DIFFICULTY_BITS = 24;
	static constexpr uint32_t HALVE_SUBSIDY_AFTER_BLOCKS_NUM = 210000;

	static constexpr int64_t LOCKTIME_THRESHOLD = 500000000;

	static constexpr uint32_t MAX_ORPHAN_BLOCKS = 50;
	static constexpr int64_t ORPHAN_BLOCK_EXPIRE_SECS = 60 * 60;

	static constexpr uint64_t INCREMENTAL_RELAY_FEE = 1000;

	static constexpr uint64_t MAX_MEMPOOL_SIZE_BYTES = 300 * 1024 * 1024;

	static constexpr uint32_t MAX_ANCESTOR_COUNT = 25;
	static constexpr uint32_t MAX_DESCENDANT_COUNT = 25;

	static constexpr uint64_t DUST_THRESHOLD = 546;

	static constexpr int64_t MEMPOOL_TX_EXPIRE_SECS = 60 * 60 * 24 * 14;

	static inline const std::string ASSUME_VALID_BLOCK_HASH{};
};
