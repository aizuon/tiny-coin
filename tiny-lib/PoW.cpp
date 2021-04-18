#include "pch.hpp"

#include <exception>
#include <algorithm>
#include <openssl/bn.h>

#include "PoW.hpp"
#include "Chain.hpp"
#include "Mempool.hpp"
#include "MerkleTree.hpp"
#include "Wallet.hpp"
#include "HashChecker.hpp"
#include "SHA256.hpp"
#include "NetParams.hpp"
#include "Utils.hpp"
#include "Log.hpp"

std::atomic_bool PoW::MineInterrupt = false;

uint8_t PoW::GetNextWorkRequired(const std::string& prevBlockHash)
{
    if (prevBlockHash.empty())
        return NetParams::INITIAL_DIFFICULTY_BITS;

    auto [prev_block, prev_block_height, prev_block_chain_idx] = Chain::LocateBlockInActiveChain(prevBlockHash);
    if ((prev_block_height + 1) % NetParams::DIFFICULTY_PERIOD_IN_BLOCKS != 0)
        return prev_block->Bits;

    std::shared_ptr<Block> period_start_block = nullptr; //HACK: this could be a ref
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
    auto newBlock = std::make_shared<Block>(*block);

    auto start = Utils::GetUnixTimestamp();
    uint64_t nonce = 0;
    BIGNUM* target_bn = HashChecker::TargetBitsToBN(newBlock->Bits);

    while (!HashChecker::IsValid(Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Utils::StringToByteArray(newBlock->Header(nonce)))), target_bn))
    {
        nonce++;
        if (nonce % 10000 == 0 && MineInterrupt)
        {
            LOG_INFO("Mining interrupted");

            MineInterrupt = false;

            BN_free(target_bn);

            return nullptr;
        }
    }

    newBlock->Nonce = nonce;
    auto duration = Utils::GetUnixTimestamp() - start;
    auto khs = (block->Nonce / duration) / 1000;
    LOG_INFO("Block found => {} s, {} KH/s, {}", duration, khs, newBlock->Id());

    return newBlock;
}

void PoW::MineForever()
{
    while (true)
    {
        auto block = AssembleAndSolveBlock();

        if (block != nullptr)
        {
            Chain::ConnectBlock(block);
            Chain::SaveToDisk();
        }
    }
}

std::shared_ptr<Block> PoW::AssembleAndSolveBlock()
{
    return AssembleAndSolveBlock(std::vector<std::shared_ptr<Tx>>());
}

std::shared_ptr<Block> PoW::AssembleAndSolveBlock(const std::vector<std::shared_ptr<Tx>>& txs)
{
    Chain::Lock.lock();
    auto prevBlockHash = !Chain::ActiveChain.empty() ? Chain::ActiveChain.back()->Id() : "";
    Chain::Lock.unlock();

    auto block = std::make_shared<Block>(0, prevBlockHash, "", Utils::GetUnixTimestamp(), GetNextWorkRequired(prevBlockHash), 0, txs);

    if (block->Txs.empty())
        block = Mempool::SelectFromMempool(block);

    auto fees = CalculateFees(block);
    auto [privKey, pubKey, myAddress] = Wallet::InitWallet();
    auto coinbaseTx = Tx::CreateCoinbase(myAddress, GetBlockSubsidy() + fees, Chain::ActiveChain.size());
    block->Txs.insert(block->Txs.begin(), coinbaseTx);
    block->MerkleHash = MerkleTree::GetRootOfTxs(block->Txs)->Value;

    if (block->Serialize().GetLength() > NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES)
        throw std::exception("Transactions specified create a block too large");

    return Mine(block);
}

std::shared_ptr<TxOut> PoW::UTXO_FromBlock(const std::shared_ptr<Block>& block, const std::shared_ptr<TxIn>& txIn)
{
    for (const auto& tx : block->Txs)
    {
        if (tx->Id() == txIn->ToSpend->TxId)
        {
            return tx->TxOuts[txIn->ToSpend->TxOutIdx];
        }
    }

    return nullptr;
}

std::shared_ptr<TxOut> PoW::Find_UTXO(const std::shared_ptr<Block>& block, const std::shared_ptr<TxIn>& txIn)
{
    for (const auto& [txOutPoint, utxo] : UTXO::Map)
    {
        if (txIn->ToSpend->TxId == utxo->TxOutPoint->TxId && txIn->ToSpend->TxOutIdx == utxo->TxOutPoint->TxOutIdx)
        {
            return utxo->TxOut;
        }
    }

    return UTXO_FromBlock(block, txIn);
}

uint64_t PoW::CalculateFees(const std::shared_ptr<Block>& block)
{
    uint64_t fee = 0;

    for (const auto& tx : block->Txs)
    {
        uint64_t spent = 0;
        for (const auto& txIn : tx->TxIns)
        {
            auto utxo = Find_UTXO(block, txIn);
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
    size_t halvings = Chain::ActiveChain.size() / NetParams::HALVE_SUBSIDY_AFTER_BLOCKS_NUM;

    if (halvings >= 64)
        return 0;

    return 50 * NetParams::COIN / pow(2, halvings);
}
