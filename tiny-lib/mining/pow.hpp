#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core/block.hpp"
#include "core/tx.hpp"
#include "util/uint256_t.hpp"

class PoW
{
public:
	static std::atomic_bool mine_interrupt;

	static uint8_t get_next_work_required(const std::string& prev_block_hash);

	static uint256_t get_block_work(uint8_t bits);

	static std::shared_ptr<Block> assemble_and_solve_block(const std::string& pay_coinbase_to_address);
	static std::shared_ptr<Block> assemble_and_solve_block(const std::string& pay_coinbase_to_address,
		const std::vector<std::shared_ptr<Tx>>& txs);

	static std::shared_ptr<Block> mine(const std::shared_ptr<Block>& block);
	static void mine_forever();

	static uint64_t calculate_fees(const std::shared_ptr<Tx>& tx);

private:
	static void mine_chunk(const BinaryBuffer& header_prefix, const uint256_t& target_hash, uint64_t start,
		uint64_t chunk_size, std::atomic_bool& found, std::atomic<uint64_t>& found_nonce,
		std::atomic<uint64_t>& hash_count);

	static uint64_t calculate_fees(const std::shared_ptr<Block>& block);
	static uint64_t get_block_subsidy();
};
