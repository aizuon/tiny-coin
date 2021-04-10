#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <tuple>
#include <utility>

#include "TxIn.hpp"
#include "TxOut.hpp"
#include "Tx.hpp"
#include "Block.hpp"

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

	static std::recursive_mutex ChainLock;

	static int32_t ActiveChainIdx;

	static int32_t GetCurrentHeight();

	static std::tuple<std::shared_ptr<Block>, int32_t, int32_t> LocateBlockInActiveChain(const std::string& blockHash);
	static  std::pair<std::shared_ptr<Block>, int32_t> LocateBlockInChain(const std::string& blockHash, const std::vector<std::shared_ptr<Block>>& chain);
};