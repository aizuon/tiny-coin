#pragma once
#include <cstdint>
#include <atomic>
#include <vector>
#include <string>
#include <memory>

#include "Tx.hpp"
#include "Block.hpp"

class PoW
{
public:
	static std::atomic_bool MineInterrupt;

	static uint8_t GetNextWorkRequired(const std::string& prevBlockHash);

	static std::shared_ptr<Block> Mine(const std::shared_ptr<Block>& block);
	static void MineForever();

private:
	static std::shared_ptr<Block> AssembleAndSolveBlock();
	static std::shared_ptr<Block> AssembleAndSolveBlock(const std::vector<std::shared_ptr<Tx>>& txs);

	static uint64_t CalculateFees(const std::shared_ptr<Block>& block);
	static uint64_t GetBlockSubsidy();
};