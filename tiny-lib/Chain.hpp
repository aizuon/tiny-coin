#pragma once
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <tuple>
#include <utility>

class Tx;
class TxIn;
class TxOut;
class Block;

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

	static std::recursive_mutex Lock;

	static int64_t ActiveChainIdx;

	static int64_t GetCurrentHeight();

	static std::tuple<std::shared_ptr<Block>, int64_t, int64_t> LocateBlockInActiveChain(const std::string& blockHash);
	static std::pair<std::shared_ptr<Block>, int64_t> LocateBlockInChain(const std::string& blockHash, const std::vector<std::shared_ptr<Block>>& chain);

	static std::shared_ptr<Block> ConnectBlock(const std::shared_ptr<Block>& block, bool doingReorg = false);

	static std::pair<std::shared_ptr<Block>, int64_t> ValidateBlock(const std::shared_ptr<Block>& block);

private:
	static int64_t GetMedianTimePast(size_t numLastBlocks);
};