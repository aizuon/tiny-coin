#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/block.hpp"
#include "core/tx.hpp"
#include "core/tx_in.hpp"
#include "core/tx_out.hpp"
#include "util/uint256_t.hpp"

struct OrphanBlock
{
	std::shared_ptr<Block> block;
	int64_t added_time;
};

class Chain
{
public:
	static const std::shared_ptr<TxIn> genesis_tx_in;
	static const std::shared_ptr<TxOut> genesis_tx_out;
	static const std::shared_ptr<Tx> genesis_tx;
	static const std::shared_ptr<Block> genesis_block;

	static std::vector<std::shared_ptr<Block>> active_chain;
	static std::vector<std::vector<std::shared_ptr<Block>>> side_branches;

	static std::unordered_multimap<std::string, OrphanBlock> orphan_blocks;

	static std::recursive_mutex mutex;

	static constexpr uint32_t ACTIVE_CHAIN_IDX = 0;

	static std::atomic_bool initial_block_download_complete;

	static std::atomic_bool assume_valid_pending;

	static uint32_t get_current_height();
	static int64_t get_median_time_past(uint32_t num_last_blocks);

	static int64_t get_median_time_past_at_height(uint32_t height, uint32_t num_last_blocks = 11);

	static uint256_t get_chain_work(const std::vector<std::shared_ptr<Block>>& chain);

	static uint32_t validate_block(const std::shared_ptr<Block>& block);

	static int64_t connect_block(const std::shared_ptr<Block>& block, bool doing_reorg = false);
	static void try_connect_orphans(const std::string& parent_block_id);
	static std::shared_ptr<Block> disconnect_block(const std::shared_ptr<Block>& block);
	static std::vector<std::shared_ptr<Block>> disconnect_to_fork(const std::shared_ptr<Block>& fork_block);

	static bool reorg_if_necessary();
	static bool try_reorg(const std::vector<std::shared_ptr<Block>>& branch, uint32_t branch_idx, uint32_t fork_idx);
	static void rollback_reorg(const std::vector<std::shared_ptr<Block>>& old_active_chain,
		const std::shared_ptr<Block>& fork_block, uint32_t branch_idx);

	static std::pair<std::shared_ptr<Block>, int64_t> locate_block_in_chain(
		const std::string& block_hash, const std::vector<std::shared_ptr<Block>>& chain);
	static std::pair<std::shared_ptr<Block>, int64_t> locate_block_in_active_chain(const std::string& block_hash);
	static std::tuple<std::shared_ptr<Block>, int64_t, int64_t> locate_block_in_all_chains(const std::string& block_hash);

	static std::tuple<std::shared_ptr<TxOut>, std::shared_ptr<Tx>, int64_t, bool, int64_t> find_tx_out_for_tx_in(
		const std::shared_ptr<TxIn>& tx_in, const std::vector<std::shared_ptr<Block>>& chain);
	static std::tuple<std::shared_ptr<TxOut>, std::shared_ptr<Tx>, int64_t, bool, int64_t>
		find_tx_out_for_tx_in_in_active_chain(const std::shared_ptr<TxIn>& tx_in);

	static void save_to_disk();
	static bool load_from_disk();

	static void reset();

private:
	static constexpr char CHAIN_PATH[] = "chain.dat";

	static std::unordered_map<std::string, uint32_t> active_chain_index;
	static uint32_t last_saved_height;

	static void index_block(const std::shared_ptr<Block>& block, uint32_t height);
	static void unindex_block(const std::shared_ptr<Block>& block);
	static void rebuild_active_chain_index();
};
