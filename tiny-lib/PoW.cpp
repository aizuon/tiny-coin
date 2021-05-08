#include "pch.hpp"
#include "PoW.hpp"

#include <exception>
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

using namespace boost::placeholders;

std::atomic_bool PoW::MineInterrupt = false;

uint8_t PoW::GetNextWorkRequired(const std::string& prevBlockHash)
{
	if (prevBlockHash.empty())
		return NetParams::INITIAL_DIFFICULTY_BITS;

	auto [prev_block, prev_block_height, prev_block_chain_idx] = Chain::LocateBlockInAllChains(prevBlockHash);
	if ((prev_block_height + 1) % NetParams::DIFFICULTY_PERIOD_IN_BLOCKS != 0)
		return prev_block->Bits;

	Chain::Mutex.lock();
	auto& period_start_block = Chain::ActiveChain[std::max(
		prev_block_height - (NetParams::DIFFICULTY_PERIOD_IN_BLOCKS - 1), 0LL)];
	Chain::Mutex.unlock();
	const int64_t actual_time_taken = prev_block->Timestamp - period_start_block->Timestamp;
	if (actual_time_taken < NetParams::DIFFICULTY_PERIOD_IN_SECS_TARGET)
		return prev_block->Bits + 1;
	if (actual_time_taken > NetParams::DIFFICULTY_PERIOD_IN_SECS_TARGET)
		return prev_block->Bits - 1;
	return prev_block->Bits;
}

std::shared_ptr<Block> PoW::AssembleAndSolveBlock(const std::string& payCoinbaseToAddress)
{
	return AssembleAndSolveBlock(payCoinbaseToAddress, {});
}

std::shared_ptr<Block> PoW::AssembleAndSolveBlock(const std::string& payCoinbaseToAddress,
                                                  const std::vector<std::shared_ptr<Tx>>& txs)
{
	Chain::Mutex.lock();
	auto prevBlockHash = !Chain::ActiveChain.empty() ? Chain::ActiveChain.back()->Id() : "";
	Chain::Mutex.unlock();

	auto block = std::make_shared<Block>(0, prevBlockHash, "", Utils::GetUnixTimestamp(),
	                                     GetNextWorkRequired(prevBlockHash), 0, txs);

	if (block->Txs.empty())
		block = Mempool::SelectFromMempool(block);

	const uint64_t fees = CalculateFees(block);
	const auto coinbaseTx = Tx::CreateCoinbase(payCoinbaseToAddress, GetBlockSubsidy() + fees,
	                                           Chain::ActiveChain.size());
	block->Txs.insert(block->Txs.begin(), coinbaseTx);
	block->MerkleHash = MerkleTree::GetRootOfTxs(block->Txs)->Value;

	if (block->Serialize().GetSize() > NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES)
		throw std::exception("Transactions specified create a block too large");

	LOG_INFO("Start mining block {} with {} fees", block->Id(), fees);
	
	return Mine(block);
}

std::shared_ptr<Block> PoW::Mine(const std::shared_ptr<Block>& block)
{
	if (MineInterrupt)
		MineInterrupt = false;

	auto newBlock = std::make_shared<Block>(*block);
	newBlock->Nonce = 0;
	BIGNUM* target_bn = HashChecker::TargetBitsToBN(newBlock->Bits);
	uint8_t num_threads = std::thread::hardware_concurrency() / 2;
	if (num_threads == 0)
		num_threads = 1;
	const uint64_t chunk_size = std::numeric_limits<uint64_t>::max() / num_threads;
	std::atomic_bool found = false;
	std::atomic<uint64_t> found_nonce = 0;
	std::atomic<uint64_t> hash_count = 0;

	const auto start = Utils::GetUnixTimestamp();
	boost::thread_group threadpool;
	for (uint8_t i = 0; i < num_threads; i++)
	{
		threadpool.create_thread(boost::bind(&PoW::MineChunk, newBlock, target_bn,
		                                     std::numeric_limits<uint64_t>::min() + chunk_size * i, chunk_size,
		                                     boost::ref(found), boost::ref(found_nonce), boost::ref(hash_count)));
	}
	threadpool.join_all();
	BN_free(target_bn);

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

	newBlock->Nonce = found_nonce;
	auto duration = Utils::GetUnixTimestamp() - start;
	if (duration == 0)
		duration = 1;
	auto khs = hash_count / duration / 1000;
	LOG_INFO("Block found => {} s, {} KH/s, {}, {}", duration, khs, newBlock->Id(), newBlock->Nonce);

	return newBlock;
}

void PoW::MineForever()
{
	if (!Chain::LoadFromDisk())
	{
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
	}

	const auto [privKey, pubKey, myAddress] = Wallet::InitWallet();
	while (true)
	{
		const auto block = AssembleAndSolveBlock(myAddress);

		if (block != nullptr)
		{
			Chain::ConnectBlock(block);
			Chain::SaveToDisk();
		}
	}
}

uint64_t PoW::CalculateFees(const std::shared_ptr<Tx>& tx)
{
	uint64_t spent = 0;
	for (const auto& txIn : tx->TxIns)
	{
		const auto utxo = UTXO::FindTxOutInMap(txIn);
		if (utxo != nullptr)
		{
			spent += utxo->Value;
		}
	}

	uint64_t sent = 0;
	for (const auto& txOut : tx->TxOuts)
	{
		sent += txOut->Value;
	}

	return spent - sent;
}

void PoW::MineChunk(const std::shared_ptr<Block>& block, BIGNUM* target_bn, uint64_t start, uint64_t chunk_size,
                    std::atomic_bool& found, std::atomic<uint64_t>& found_nonce, std::atomic<uint64_t>& hash_count)
{
	uint64_t i = 0;
	while (!HashChecker::IsValid(
		Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(block->Header(start + i)))),
		target_bn))
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

uint64_t PoW::CalculateFees(const std::shared_ptr<Block>& block)
{
	uint64_t fee = 0;

	for (const auto& tx : block->Txs)
	{
		uint64_t spent = 0;
		for (const auto& txIn : tx->TxIns)
		{
			const auto utxo = UTXO::FindTxOutInMapOrBlock(block, txIn);
			if (utxo != nullptr)
			{
				spent += utxo->Value;
			}
		}

		uint64_t sent = 0;
		for (const auto& txOut : tx->TxOuts)
		{
			sent += txOut->Value;
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

	return 50 * NetParams::COIN / pow(2, halvings);
}
