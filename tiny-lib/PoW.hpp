#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <openssl/bn.h>

#include "Block.hpp"
#include "Tx.hpp"

class PoW
{
public:
	static std::atomic_bool MineInterrupt;

	static uint8_t GetNextWorkRequired(const std::string& prevBlockHash);

	static std::shared_ptr<Block> AssembleAndSolveBlock(const std::string& payCoinbaseToAddress);
	static std::shared_ptr<Block> AssembleAndSolveBlock(const std::string& payCoinbaseToAddress,
	                                                    const std::vector<std::shared_ptr<Tx>>& txs);

	static std::shared_ptr<Block> Mine(const std::shared_ptr<Block>& block);
	static void MineForever();

private:
	static void MineChunk(const std::shared_ptr<Block>& block, BIGNUM* target_bn, uint64_t start, uint64_t chunk_size,
	                      std::atomic_bool& found, std::atomic<uint64_t>& found_nonce,
	                      std::atomic<uint64_t>& hash_count);

	static std::shared_ptr<TxOut>
	UTXO_FromBlock(const std::shared_ptr<Block>& block, const std::shared_ptr<TxIn>& txIn);
	static std::shared_ptr<TxOut> Find_UTXO(const std::shared_ptr<Block>& block, const std::shared_ptr<TxIn>& txIn);

	static uint64_t CalculateFees(const std::shared_ptr<Block>& block);
	static uint64_t GetBlockSubsidy();
};
