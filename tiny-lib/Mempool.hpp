#pragma once
#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <memory>

class Tx;
class TxIn;
class TxOutPoint;
class UnspentTxOut;
class Block;

class Mempool
{
public:
	static std::unordered_map<std::string, std::shared_ptr<Tx>> Map;

	static std::vector<std::shared_ptr<Tx>> OrphanedTxs;

	static std::shared_ptr<UnspentTxOut> Find_UTXO_InMempool(const std::shared_ptr<TxOutPoint>& txOutPoint);

	static std::shared_ptr<Block> SelectFromMempool(std::shared_ptr<Block>& block);

private:
	static bool CheckBlockSize(const std::shared_ptr<Block>& block);

	static std::shared_ptr<Block> TryAddToBlock(std::shared_ptr<Block>& block, const std::string& txId, std::set<std::string>& addedToBlock);
};
