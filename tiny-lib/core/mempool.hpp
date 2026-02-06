#pragma once
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/block.hpp"
#include "core/tx.hpp"
#include "core/tx_out_point.hpp"
#include "core/unspent_tx_out.hpp"

class Mempool
{
public:
	static std::unordered_map<std::string, std::shared_ptr<Tx>> map;

	static std::vector<std::shared_ptr<Tx>> orphaned_txs;

	static std::recursive_mutex mutex;

	static std::shared_ptr<UTXO> find_utxo_in_mempool(const std::shared_ptr<TxOutPoint>& tx_out_point);

	static std::shared_ptr<Block> select_from_mempool(const std::shared_ptr<Block>& block);

	static void add_tx_to_mempool(const std::shared_ptr<Tx>& tx);

	static bool try_replace_by_fee(const std::shared_ptr<Tx>& tx);

private:
	static bool check_block_size(uint32_t current_size);

	static std::shared_ptr<Block> try_add_to_block(std::shared_ptr<Block> block, const std::string& tx_id,
		std::set<std::string>& added_to_block, uint32_t& current_block_size);

	static std::vector<std::shared_ptr<Tx>> find_conflicting_txs(const std::shared_ptr<Tx>& tx);
	static std::vector<std::string> find_descendant_tx_ids(const std::string& tx_id);
};
