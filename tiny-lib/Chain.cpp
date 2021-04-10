#include "pch.hpp"

#include "Chain.hpp"

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
	if (std::get<0>(located_block) != nullptr)
		return std::make_tuple(std::get<0>(located_block), std::get<1>(located_block), chain_idx);

	for (const auto& side_chain : SideBranches)
	{
		chain_idx++;

		located_block = LocateBlockInChain(blockHash, side_chain);
		if (std::get<0>(located_block) != nullptr)
			return std::make_tuple(std::get<0>(located_block), std::get<1>(located_block), chain_idx);
	}

	return std::make_tuple(nullptr, -1, -1);
}

std::tuple<std::shared_ptr<Block>, int32_t> Chain::LocateBlockInChain(const std::string& blockHash, const std::vector<std::shared_ptr<Block>>& chain)
{
	std::lock_guard lock(ChainLock);

	int32_t height = 0;
	for (const auto& block : chain)
	{
		if (block->Id() == blockHash)
		{
			return std::make_tuple(block, height);
		}

		height++;
	}

	return std::make_tuple(nullptr, -1);
}
