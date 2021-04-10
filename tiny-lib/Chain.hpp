#pragma once
#include <vector>
#include <memory>
#include <mutex>

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

	static std::mutex ChainLock;
};