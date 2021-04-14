#include "pch.hpp"

#include <exception>
#include <algorithm>

#include "Chain.hpp"
#include "NetParams.hpp"

const std::shared_ptr<TxIn> Chain::GenesisTxIn = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{ 0x00 }, std::vector<uint8_t>(), 0);
const std::shared_ptr<TxOut> Chain::GenesisTxOut = std::make_shared<TxOut>(5000000000, "143UVyz7ooiAv1pMqbwPPpnH4BV9ifJGFF");
const std::shared_ptr<Tx> Chain::GenesisTx = std::make_shared<Tx>(std::vector<std::shared_ptr<TxIn>>{ Chain::GenesisTxIn },
	std::vector<std::shared_ptr<TxOut>>{ Chain::GenesisTxOut }, 0);
const std::shared_ptr<Block> Chain::GenesisBlock = std::make_shared<Block>(0, "", "7118894203235a955a908c0abfc6d8fe6edec47b0a04ce1bf7263da3b4366d22",
	1501821412, 24, 10126761, std::vector<std::shared_ptr<Tx>>{ GenesisTx });

std::vector<std::shared_ptr<Block>> Chain::ActiveChain = std::vector<std::shared_ptr<Block>>{ GenesisBlock };
std::vector<std::vector<std::shared_ptr<Block>>> Chain::SideBranches = std::vector<std::vector<std::shared_ptr<Block>>>{ };
std::vector<std::shared_ptr<Block>> Chain::OrphanBlocks = std::vector<std::shared_ptr<Block>>{ };

std::recursive_mutex Chain::ChainLock;

int32_t Chain::ActiveChainIdx;

int32_t Chain::GetCurrentHeight()
{
	std::lock_guard lock(ChainLock);

	return ActiveChain.size();
}

std::tuple<std::shared_ptr<Block>, int32_t, int32_t> Chain::LocateBlockInActiveChain(const std::string& blockHash)
{
	std::lock_guard lock(ChainLock);

	int32_t chain_idx = 0;
	auto located_block = LocateBlockInChain(blockHash, ActiveChain);
	if (located_block.first != nullptr)
		return std::make_tuple(located_block.first, located_block.second, chain_idx);

	for (const auto& side_chain : SideBranches)
	{
		chain_idx++;

		located_block = LocateBlockInChain(blockHash, side_chain);
		if (std::get<0>(located_block) != nullptr)
			return std::make_tuple(located_block.first, located_block.second, chain_idx);
	}

	return std::make_tuple(nullptr, -1, -1);
}

std::pair<std::shared_ptr<Block>, int32_t> Chain::LocateBlockInChain(const std::string& blockHash, const std::vector<std::shared_ptr<Block>>& chain)
{
	std::lock_guard lock(ChainLock);

	int32_t height = 0;
	for (const auto& block : chain)
	{
		if (block->Id() == blockHash)
		{
			return std::make_pair(block, height);
		}

		height++;
	}

	return std::make_pair(nullptr, -1);
}

std::shared_ptr<Block> Chain::ConnectBlock(const std::shared_ptr<Block>& block, bool doingReorg /*= false*/)
{
	std::lock_guard lock(ChainLock);

	if (doingReorg)
	{
		if (std::get<0>(LocateBlockInActiveChain(block->Id())) != nullptr)
		{
			return nullptr;
		}
	}

	//TODO: validate block
}

std::shared_ptr<Block> Chain::ValidateBlock(const std::shared_ptr<Block>& block)
{
	if (block->Txs.empty())
		throw std::exception("Chain::ValidateBlock --- block->Txs.empty()");

	auto now = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	if (block->Timestamp - now > NetParams::MAX_FUTURE_BLOCK_TIME_IN_SECS)
		throw std::exception("Chain::ValidateBlock --- block->Timestamp - now > NetParams::MAX_FUTURE_BLOCK_TIME_IN_SECS");

	if (std::stoul(block->Id(), nullptr, 16) > (1 << (256 - block->Bits)))
		throw std::exception("Chain::ValidateBlock --- std::stoul(block->Id(), nullptr, 16) > (1 << (256 - block->Bits)");

	auto coinbase_pred = [](const std::shared_ptr<Tx>& tx)
	{
		return tx->IsCoinbase();
	};
	auto coinbase_it = std::find_if(block->Txs.begin(), block->Txs.end(), coinbase_pred);
	if (coinbase_it != block->Txs.begin() || std::find_if(++coinbase_it, block->Txs.end(), coinbase_pred) != block->Txs.end())
		throw std::exception("Chain::ValidateBlock --- coinbase_it != block->Txs.begin() || std::find_if(++coinbase_it, block->Txs.end(), coinbase_pred) != block->Txs.end()");

	try
	{
		for (int i = 0; i < block->Txs.size(); i++)
		{
			block->Txs[i]->Validate(i == 0);
		}
	}
	catch (...)
	{
		throw std::exception("Chain::ValidateBlock --- block->Txs[i]->Validate(i == 0)");
	}

	//TODO: check merkle root
}
