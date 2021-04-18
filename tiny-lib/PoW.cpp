#include "pch.hpp"

#include <algorithm>

#include "PoW.hpp"
#include "NetParams.hpp"
#include "Chain.hpp"

uint8_t PoW::GetNextWorkRequired(const std::string& prevBlockHash)
{
    if (prevBlockHash.empty())
        return NetParams::INITIAL_DIFFICULTY_BITS;

    auto [prev_block, prev_block_height, prev_block_chain_idx] = Chain::LocateBlockInActiveChain(prevBlockHash);
    if ((prev_block_height + 1) % NetParams::DIFFICULTY_PERIOD_IN_BLOCKS != 0)
        return prev_block->Bits;

    std::shared_ptr<Block> period_start_block = nullptr; //HACK: this chould be a ref
    {
        std::lock_guard lock(Chain::Lock);

        period_start_block = Chain::ActiveChain[std::max(prev_block_height - (NetParams::DIFFICULTY_PERIOD_IN_BLOCKS - 1), 0ULL)];
    }
    int64_t actual_time_taken = prev_block->Timestamp - period_start_block->Timestamp;
    if (actual_time_taken < NetParams::DIFFICULTY_PERIOD_IN_SECS_TARGET)
        return prev_block->Bits + 1;
    else if (actual_time_taken > NetParams::DIFFICULTY_PERIOD_IN_SECS_TARGET)
        return prev_block->Bits - 1;
    else
        return prev_block->Bits;
}

std::shared_ptr<Block> PoW::Mine(const std::shared_ptr<Block>& block)
{
    //TODO

    return nullptr;
}

void PoW::MineForever()
{
    //TODO
}

std::shared_ptr<Block> PoW::AssembleAndSolveBlock()
{
    return AssembleAndSolveBlock(std::vector<std::shared_ptr<Tx>>());
}

std::shared_ptr<Block> PoW::AssembleAndSolveBlock(const std::vector<std::shared_ptr<Tx>>& txs)
{
    //TODO

    return nullptr;
}

uint64_t PoW::CalculateFees(const std::shared_ptr<Block>& block)
{
    //TODO

    return 0;
}

uint64_t PoW::GetBlockSubsidy()
{
    //TODO

    return 0;
}
