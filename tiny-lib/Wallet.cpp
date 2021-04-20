#include "pch.hpp"

#include <thread>
#include <chrono>
#include <unordered_set>
#include <algorithm>
#include <fstream>
#include <iterator>

#include "Wallet.hpp"
#include "Log.hpp"
#include "NetParams.hpp"
#include "Utils.hpp"
#include "Base58.hpp"
#include "ECDSA.hpp"
#include "RIPEMD160.hpp"
#include "SHA256.hpp"
#include "GetActiveChainMsg.hpp"
#include "GetMempoolMsg.hpp"
#include "GetUTXOsMsg.hpp"
#include "MsgCache.hpp"
#include "MsgSerializer.hpp"
#include "NetClient.hpp"
#include "SendActiveChainMsg.hpp"
#include "SendMempoolMsg.hpp"
#include "SendUTXOsMsg.hpp"
#include "TxInfoMsg.hpp"
#include "Tx.hpp"
#include "UnspentTxOut.hpp"

std::string Wallet::WalletPath = DefaultWalletPath;

std::string Wallet::PubKeyToAddress(const std::vector<uint8_t>& pubKey)
{
    auto sha256 = SHA256::HashBinary(pubKey);

    auto ripe = RIPEMD160::HashBinary(sha256);

    ripe.insert(ripe.begin(), 0x00);

    auto sha256d = SHA256::DoubleHashBinary(ripe);

    std::vector<uint8_t> checksum(sha256d.begin(), sha256d.begin() + 4);

    ripe.insert(ripe.end(), checksum.begin(), checksum.end());

    return PubKeyHashVersion + Base58::Encode(ripe);
}

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> Wallet::InitWallet(const std::string& walletPath)
{
    WalletPath = walletPath;

    std::vector<uint8_t> privKey;
    std::vector<uint8_t> pubKey;
    std::string address;

    std::ifstream wallet_in(walletPath, std::ios::binary);
    if (wallet_in.good())
    {
        privKey = std::vector<uint8_t>(std::istreambuf_iterator<char>(wallet_in), {});
        pubKey = ECDSA::GetPubKeyFromPrivKey(privKey);
        address = PubKeyToAddress(pubKey);
        wallet_in.close();
    }
    else
    {
        LOG_INFO("Generating new wallet {}", walletPath);

        auto [privKey, pubKey] = ECDSA::Generate();
        address = PubKeyToAddress(pubKey);

        std::ofstream wallet_out(walletPath, std::ios::binary);
        wallet_out.write((const char*)privKey.data(), privKey.size());
        wallet_out.flush();
        wallet_out.close();
    }

    static bool printedAddress = false;
    if (!printedAddress)
    {
        printedAddress = true;

        LOG_INFO("Your address is {}", address);
    }

    return { privKey, pubKey, address };
}

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> Wallet::InitWallet()
{
    return InitWallet(WalletPath);
}

std::shared_ptr<TxIn> Wallet::MakeTxIn(const std::vector<uint8_t>& privKey, const std::shared_ptr<TxOutPoint>& txOutPoint, const std::shared_ptr<TxOut>& txOut)
{
    int32_t sequence = 0;

    auto pubKey = ECDSA::GetPubKeyFromPrivKey(privKey);
    auto spend_msg = MsgSerializer::BuildSpendMsg(txOutPoint, pubKey, sequence, std::vector<std::shared_ptr<TxOut>>{ txOut });
    auto unlock_sig = ECDSA::SignMsg(spend_msg, privKey);

    return std::make_shared<TxIn>(txOutPoint, unlock_sig, pubKey, sequence);
}

void Wallet::SendValue(uint64_t value, const std::string& address, const std::vector<uint8_t>& privKey)
{
    auto pubKey = ECDSA::GetPubKeyFromPrivKey(privKey);
    auto myAddress = PubKeyToAddress(pubKey);
    auto my_coins = FindUTXOsForAddress(myAddress);
    if (my_coins.empty())
    {
        LOG_ERROR("No coins found");

        return;
    }
    std::sort(my_coins.begin(), my_coins.end(),
        [](const std::shared_ptr<UnspentTxOut>& a, const std::shared_ptr<UnspentTxOut>& b) -> bool
        {
            if (a->TxOut->Value > b->TxOut->Value)
                return true;
            else if (a->TxOut->Value == b->TxOut->Value)
                return a->Height > b->Height;
            else
                return false;
        });
    std::unordered_set<std::shared_ptr<UnspentTxOut>> selected_coins;
    for (const auto& coin : my_coins)
    {
        if (!selected_coins.contains(coin))
        {
            selected_coins.insert(selected_coins.end(), coin);
        }
        uint64_t sum = 0;
        for (const auto& selected_coin : selected_coins)
        {
            sum += selected_coin->TxOut->Value;
        }
        if (sum > value)
        {
            break;
        }
    }
    auto txOut = std::make_shared<TxOut>(value, address);
    std::vector<std::shared_ptr<TxIn>> txIns;
    for (const auto& selected_coin : selected_coins)
    {
        txIns.push_back(MakeTxIn(privKey, selected_coin->TxOutPoint, txOut));
    }
    auto tx = std::make_shared<Tx>(txIns, std::vector<std::shared_ptr<TxOut>>{ txOut }, -1);
    LOG_INFO("Built transaction {}, broadcasting", tx->Id());
    if (!NetClient::SendMsgRandom(TxInfoMsg(tx)))
    {
        LOG_ERROR("No connection to send transaction");
    }
}

void Wallet::PrintTxStatus(const std::string& txId)
{
    if (MsgCache::SendMempoolMsg != nullptr)
        MsgCache::SendMempoolMsg = nullptr;

    if (!NetClient::SendMsgRandom(GetMempoolMsg()))
    {
        LOG_ERROR("No connection to ask mempool");

        return;
    }

    auto start = Utils::GetUnixTimestamp();
    while (MsgCache::SendMempoolMsg == nullptr)
    {
        if (Utils::GetUnixTimestamp() - start > MsgCache::MAX_MSG_AWAIT_TIME_IN_SECS)
        {
            LOG_ERROR("Timeout on GetMempoolMsg");

            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    for (const auto& tx : MsgCache::SendMempoolMsg->Mempool)
    {
        if (tx == txId)
        {
            LOG_INFO("Transaction {} is in mempool", txId);

            return;
        }
    }

    if (MsgCache::SendActiveChainMsg != nullptr)
        MsgCache::SendActiveChainMsg = nullptr;

    if (!NetClient::SendMsgRandom(GetActiveChainMsg()))
    {
        LOG_ERROR("No connection to ask active chain");

        return;
    }

    start = Utils::GetUnixTimestamp();
    while (MsgCache::SendActiveChainMsg == nullptr)
    {
        if (Utils::GetUnixTimestamp() - start > MsgCache::MAX_MSG_AWAIT_TIME_IN_SECS)
        {
            LOG_ERROR("Timeout on GetActiveChainMsg");

            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    for (int64_t height = 0; height < MsgCache::SendActiveChainMsg->ActiveChain.size(); height++)
    {
        const auto& block = MsgCache::SendActiveChainMsg->ActiveChain[height];
        for (const auto& tx : block->Txs)
        {
            if (tx->Id() == txId)
            {
                LOG_INFO("Transaction {} is mined in {} at height {}", txId, block->Id(), height);

                return;
            }
        }
    }

    LOG_INFO("Transaction {} not found", txId);
}

uint64_t Wallet::GetBalance(const std::string& address)
{
    auto utxos = FindUTXOsForAddress(address);
    uint64_t value = 0;
    for (const auto& utxo : utxos)
        value += utxo->TxOut->Value;

    LOG_INFO("Address {} holds {} coins", address, value / NetParams::COIN);

    return value;
}

std::vector<std::shared_ptr<UnspentTxOut>> Wallet::FindUTXOsForAddress(const std::string& address)
{
    if (MsgCache::SendUTXOsMsg != nullptr)
        MsgCache::SendUTXOsMsg = nullptr;

    if (!NetClient::SendMsgRandom(GetUTXOsMsg()))
    {
        LOG_ERROR("No connection to ask UTXO set");

        return std::vector<std::shared_ptr<UnspentTxOut>>();
    }

    auto start = Utils::GetUnixTimestamp();
    while (MsgCache::SendUTXOsMsg == nullptr)
    {
        if (Utils::GetUnixTimestamp() - start > MsgCache::MAX_MSG_AWAIT_TIME_IN_SECS)
        {
            LOG_ERROR("Timeout on GetUTXOsMsg");

            return std::vector<std::shared_ptr<UnspentTxOut>>();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::vector<std::shared_ptr<UnspentTxOut>> utxos;
    for (const auto& [k, v] : MsgCache::SendUTXOsMsg->UTXO_Map)
    {
        if (v->TxOut->ToAddress == address)
        {
            utxos.push_back(v);
        }
    }
    return utxos;
}
