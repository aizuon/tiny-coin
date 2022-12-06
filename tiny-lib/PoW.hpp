#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Block.hpp"
#include "Tx.hpp"
#include "uint256_t.hpp"

class PoW
{
public:
	static std::atomic_bool MineInterrupt;

	static uint8_t GetNextWorkRequired(const std::string& prev_block_hash);

	static std::shared_ptr<Block> AssembleAndSolveBlock(const std::string& pay_coinbase_to_address);
	static std::shared_ptr<Block> AssembleAndSolveBlock(const std::string& pay_coinbase_to_address,
	                                                    const std::vector<std::shared_ptr<Tx>>& txs);

	static std::shared_ptr<Block> Mine(std::shared_ptr<Block> block);
	static void MineForever();

	static uint64_t CalculateFees(std::shared_ptr<Tx> tx);

private:
	static void MineChunk(std::shared_ptr<Block> block, const uint256_t& target_hash, uint64_t start,
	                      uint64_t chunk_size, std::atomic_bool& found, std::atomic<uint64_t>& found_nonce,
	                      std::atomic<uint64_t>& hash_count);

	static uint64_t CalculateFees(std::shared_ptr<Block> block);
	static uint64_t GetBlockSubsidy();
};
