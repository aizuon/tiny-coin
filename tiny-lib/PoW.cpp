#include "pch.hpp"
#include "PoW.hpp"

#include <cmath>
#include <exception>
#include <limits>
#include <ranges>
#include <boost/bind/bind.hpp>
#include <boost/thread/thread.hpp>

#include "Chain.hpp"
#include "GetBlockMsg.hpp"
#include "HashChecker.hpp"
#include "Log.hpp"
#include "Mempool.hpp"
#include "MerkleTree.hpp"
#include "NetClient.hpp"
#include "NetParams.hpp"
#include "SHA256.hpp"
#include "Utils.hpp"
#include "Wallet.hpp"

std::atomic_bool PoW::MineInterrupt = false;

uint8_t PoW::GetNextWorkRequired(const std::string& prev_block_hash)
{
	if (prev_block_hash.empty())
		return NetParams::INITIAL_DIFFICULTY_BITS;

	auto [prev_block, prev_block_height, prev_block_chain_idx] = Chain::LocateBlockInAllChains(prev_block_hash);
	if ((prev_block_height + 1) % NetParams::DIFFICULTY_PERIOD_IN_BLOCKS != 0)
		return prev_block->Bits;

	Chain::Mutex.lock();
	const auto& period_start_block = Chain::ActiveChain[std::max(
		prev_block_height - (NetParams::DIFFICULTY_PERIOD_IN_BLOCKS - 1), 0LL)];
	Chain::Mutex.unlock();
	const int64_t actual_time_taken = prev_block->Timestamp - period_start_block->Timestamp;
	if (actual_time_taken < NetParams::DIFFICULTY_PERIOD_IN_SECS_TARGET)
		return prev_block->Bits + 1;
	if (actual_time_taken > NetParams::DIFFICULTY_PERIOD_IN_SECS_TARGET)
		return prev_block->Bits - 1;
	return prev_block->Bits;
}

std::shared_ptr<Block> PoW::AssembleAndSolveBlock(const std::string& pay_coinbase_to_address)
{
	return AssembleAndSolveBlock(pay_coinbase_to_address, {});
}

std::shared_ptr<Block> PoW::AssembleAndSolveBlock(const std::string& pay_coinbase_to_address,
                                                  const std::vector<std::shared_ptr<Tx>>& txs)
{
	Chain::Mutex.lock();
	auto prev_block_hash = !Chain::ActiveChain.empty() ? Chain::ActiveChain.back()->Id() : "";
	Chain::Mutex.unlock();

	auto block = std::make_shared<Block>(0, prev_block_hash, "", Utils::GetUnixTimestamp(),
	                                     GetNextWorkRequired(prev_block_hash), 0, txs);

	if (block->Txs.empty())
		block = Mempool::SelectFromMempool(block);

	const uint64_t fees = CalculateFees(block);
	const auto coinbase_tx = Tx::CreateCoinbase(pay_coinbase_to_address, GetBlockSubsidy() + fees,
	                                           Chain::ActiveChain.size());
	block->Txs.insert(block->Txs.begin(), coinbase_tx);
	block->MerkleHash = MerkleTree::GetRootOfTxs(block->Txs)->Value;

	if (block->Serialize().GetSize() > NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES)
		throw std::exception("Transactions specified create a block too large");

	LOG_INFO("Start mining block {} with {} fees", block->Id(), fees);

	return Mine(block);
}

std::shared_ptr<Block> PoW::Mine(std::shared_ptr<Block> block)
{
	if (MineInterrupt)
		MineInterrupt = false;

	auto new_block = std::make_shared<Block>(*block);
	new_block->Nonce = 0;
	const uint256_t target_hash = uint256_t(1) << (std::numeric_limits<uint8_t>::max() - new_block->Bits);
	int8_t num_threads = boost::thread::hardware_concurrency() - 3;
	if (num_threads <= 0)
		num_threads = 1;
	const uint64_t chunk_size = std::numeric_limits<uint64_t>::max() / num_threads;
	std::atomic_bool found = false;
	std::atomic<uint64_t> found_nonce = 0;
	std::atomic<uint64_t> hash_count = 0;

	const auto start = Utils::GetUnixTimestamp();
	boost::thread_group thread_pool;
	for (uint8_t i = 0; i < num_threads; i++)
	{
		thread_pool.create_thread(boost::bind(&PoW::MineChunk, new_block, target_hash,
		                                     std::numeric_limits<uint64_t>::min() + chunk_size * i, chunk_size,
		                                     boost::ref(found), boost::ref(found_nonce), boost::ref(hash_count)));
	}
	thread_pool.join_all();

	if (MineInterrupt)
	{
		MineInterrupt = false;

		LOG_INFO("Mining interrupted");

		return nullptr;
	}

	if (!found)
	{
		LOG_ERROR("No nonce satisfies required bits");

		return nullptr;
	}

	new_block->Nonce = found_nonce;
	auto duration = Utils::GetUnixTimestamp() - start;
	if (duration == 0)
		duration = 1;
	auto khs = hash_count / duration / 1000;
	LOG_INFO("Block found => {} s, {} kH/s, {}, {}", duration, khs, new_block->Id(), new_block->Nonce);

	return new_block;
}

void PoW::MineForever()
{
	Chain::LoadFromDisk();

	if (NetClient::SendMsgRandom(GetBlockMsg(Chain::ActiveChain.back()->Id())))
	{
		LOG_INFO("Starting initial block sync");

		const auto start = Utils::GetUnixTimestamp();
		while (!Chain::InitialBlockDownloadComplete)
		{
			if (Utils::GetUnixTimestamp() - start > 60)
			{
				//TODO: if sync has started but hasnt finished in time, cancel sync and reset chain

				LOG_ERROR("Timeout on initial block sync");

				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
	}

	const auto [priv_key, pub_key, my_address] = Wallet::InitWallet();
	while (true)
	{
		const auto block = AssembleAndSolveBlock(my_address);

		if (block != nullptr)
		{
			Chain::ConnectBlock(block);
			Chain::SaveToDisk();
		}
	}
}

uint64_t PoW::CalculateFees(std::shared_ptr<Tx> tx)
{
	uint64_t spent = 0;
	for (const auto& tx_in : tx->TxIns)
	{
		const auto utxo = UTXO::FindTxOutInMap(tx_in);
		if (utxo != nullptr)
		{
			spent += utxo->Value;
		}
	}

	uint64_t sent = 0;
	for (const auto& tx_out : tx->TxOuts)
	{
		sent += tx_out->Value;
	}

	return spent - sent;
}

void PoW::MineChunk(std::shared_ptr<Block> block, const uint256_t& target_hash, uint64_t start,
                    uint64_t chunk_size, std::atomic_bool& found, std::atomic<uint64_t>& found_nonce,
                    std::atomic<uint64_t>& hash_count)
{
	uint64_t i = 0;
	while (!HashChecker::IsValid(
		Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(block->Header(start + i).GetBuffer())),
		target_hash))
	{
		++hash_count;

		i++;
		if (found || i == chunk_size || MineInterrupt)
		{
			return;
		}
	}
	found = true;
	found_nonce = start + i;
}

uint64_t PoW::CalculateFees(std::shared_ptr<Block> block)
{
	uint64_t fee = 0;

	for (const auto& tx : block->Txs)
	{
		uint64_t spent = 0;
		for (const auto& tx_in : tx->TxIns)
		{
			const auto utxo = UTXO::FindTxOutInMapOrBlock(block, tx_in);
			if (utxo != nullptr)
			{
				spent += utxo->Value;
			}
		}

		uint64_t sent = 0;
		for (const auto& tx_out : tx->TxOuts)
		{
			sent += tx_out->Value;
		}

		fee += spent - sent;
	}

	return fee;
}

uint64_t PoW::GetBlockSubsidy()
{
	const uint32_t halvings = Chain::ActiveChain.size() / NetParams::HALVE_SUBSIDY_AFTER_BLOCKS_NUM;

	if (halvings >= 64)
		return 0;

	return 50 * NetParams::COIN / std::pow(2, halvings);
}
