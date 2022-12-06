#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Block.hpp"
#include "Tx.hpp"
#include "TxIn.hpp"
#include "TxOut.hpp"

class Chain
{
public:
	static const std::shared_ptr<TxIn> GenesisTxIn;
	static const std::shared_ptr<TxOut> GenesisTxOut;
	static const std::shared_ptr<Tx> GenesisTx;
	static const std::shared_ptr<Block> GenesisBlock;

	static std::vector<std::shared_ptr<Block>> ActiveChain;
	static std::vector<std::vector<std::shared_ptr<Block>>> SideBranches;

	static std::vector<std::shared_ptr<Block>> OrphanBlocks;

	static std::recursive_mutex Mutex;

	static constexpr uint32_t ActiveChainIdx = 0;

	static std::atomic_bool InitialBlockDownloadComplete;

	static uint32_t GetCurrentHeight();
	static int64_t GetMedianTimePast(uint32_t num_last_blocks);

	static uint32_t ValidateBlock(std::shared_ptr<Block> block);

	static int64_t ConnectBlock(std::shared_ptr<Block> block, bool doing_reorg = false);
	static std::shared_ptr<Block> DisconnectBlock(std::shared_ptr<Block> block);
	static std::vector<std::shared_ptr<Block>> DisconnectToFork(std::shared_ptr<Block> fork_block);

	static bool ReorgIfNecessary();
	static bool TryReorg(const std::vector<std::shared_ptr<Block>>& branch, uint32_t branch_idx, uint32_t fork_idx);
	static void RollbackReorg(const std::vector<std::shared_ptr<Block>>& old_active_chain,
	                          std::shared_ptr<Block> fork_block, uint32_t branch_idx);

	static std::pair<std::shared_ptr<Block>, int64_t> LocateBlockInChain(
		const std::string& block_hash, const std::vector<std::shared_ptr<Block>>& chain);
	static std::tuple<std::shared_ptr<Block>, int64_t> LocateBlockInActiveChain(const std::string& block_hash);
	static std::tuple<std::shared_ptr<Block>, int64_t, int64_t> LocateBlockInAllChains(const std::string& block_hash);

	static std::tuple<std::shared_ptr<TxOut>, std::shared_ptr<Tx>, int64_t, bool, int64_t> FindTxOutForTxIn(
		std::shared_ptr<TxIn> tx_in, const std::vector<std::shared_ptr<Block>>& chain);
	static std::tuple<std::shared_ptr<TxOut>, std::shared_ptr<Tx>, int64_t, bool, int64_t>
	FindTxOutForTxInInActiveChain(std::shared_ptr<TxIn> tx_in);

	static void SaveToDisk();
	static bool LoadFromDisk();

private:
	static constexpr char ChainPath[] = "chain.dat";
};
