#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core/block.hpp"
#include "core/tx.hpp"
#include "mining/i_mining_backend.hpp"
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

	static std::vector<uint8_t> target_to_bytes(const uint256_t& target);

private:
	static std::unique_ptr<IMiningBackend> mining_backend_;
	static IMiningBackend& get_backend();

	static uint64_t calculate_fees(const std::shared_ptr<Block>& block);
	static uint64_t get_block_subsidy();
};
