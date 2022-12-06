#pragma once
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "Block.hpp"
#include "Tx.hpp"
#include "TxOutPoint.hpp"
#include "UnspentTxOut.hpp"

class Mempool
{
public:
	static std::unordered_map<std::string, std::shared_ptr<Tx>> Map;

	static std::vector<std::shared_ptr<Tx>> OrphanedTxs;

	static std::recursive_mutex Mutex;

	static std::shared_ptr<UTXO> Find_UTXO_InMempool(std::shared_ptr<TxOutPoint> tx_out_point);

	static std::shared_ptr<Block> SelectFromMempool(std::shared_ptr<Block> block);

	static void AddTxToMempool(std::shared_ptr<Tx> tx);

private:
	static bool CheckBlockSize(std::shared_ptr<Block> block);

	static std::shared_ptr<Block> TryAddToBlock(std::shared_ptr<Block> block, const std::string& tx_id,
	                                            std::set<std::string>& added_to_block);
};
