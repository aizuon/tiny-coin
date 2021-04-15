#include "pch.hpp"

#include <exception>
#include <fmt/format.h>

#include "Tx.hpp"
#include "NetParams.hpp"
#include "Utils.hpp"
#include "SHA256d.hpp"
#include "BinaryBuffer.hpp"
#include "UnspentTxOut.hpp"
#include "Chain.hpp"

Tx::Tx(const std::vector<std::shared_ptr<TxIn>>& txIns, const std::vector<std::shared_ptr<TxOut>>& txOuts, int64_t lockTime)
    : TxIns(txIns), TxOuts(txOuts), LockTime(lockTime)
{
}

bool Tx::IsCoinbase() const
{
    return TxIns.size() == 1 && TxIns.front()->ToSpend == nullptr;
}

std::string Tx::Id() const
{
    return Utils::ByteArrayToHexString(SHA256d::HashBinary(Serialize()));
}

void Tx::ValidateBasics(bool coinbase /*= false*/) const
{
    if (TxOuts.empty() || (TxIns.empty() && !coinbase))
        throw std::exception("Tx::Validate --- TxOuts.empty() || (TxIns.empty() && !coinbase)");

    if (Serialize().size() > NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES)
        throw std::exception("Tx::Validate --- Serialize().size() > NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES");

    uint64_t totalSpent = 0;
    for (const auto& tx_out : TxOuts)
        totalSpent += tx_out->Value;

    if (totalSpent > NetParams::MAX_MONEY)
        throw std::exception("Tx::Validate --- totalSpent > NetParams::MAX_MONEY");
}

std::vector<uint8_t> Tx::Serialize() const
{
    BinaryBuffer buffer;

    buffer.Write(TxIns.size());
    for (const auto& tx_in : TxIns)
        buffer.Write(tx_in->Serialize());

    buffer.Write(TxOuts.size());
    for (const auto& tx_out : TxOuts)
        buffer.Write(tx_out->Serialize());

    buffer.Write(LockTime);

    return buffer.GetBuffer();
}

std::shared_ptr<Tx> Tx::CreateCoinbase(const std::string& PayToAddr, uint64_t value, int64_t height)
{
    BinaryBuffer tx_in_unlockSig(sizeof(height));
    tx_in_unlockSig.Write(height);
    auto tx_in = std::make_shared<TxIn>(nullptr, tx_in_unlockSig.GetBuffer(), std::vector<uint8_t>(), -1);

    auto tx_out = std::make_shared<TxOut>(value, PayToAddr);

    std::vector<std::shared_ptr<TxIn>> tx_ins{ tx_in };
    std::vector<std::shared_ptr<TxOut>> tx_outs{ tx_out };
    auto tx = std::make_shared<Tx>(tx_ins, tx_outs, -1);

    return tx;
}

void Tx::Validate(const std::shared_ptr<Tx>& tx, const ValidateRequest& req)
{
    tx->ValidateBasics(req.AsCoinbase);

    uint64_t avaliableToSpend = 0;
    const auto& txIns = tx->TxIns;
    for (size_t i = 0; i < txIns.size(); i++)
    {
        const auto& txIn = txIns[i];

        std::shared_ptr<UnspentTxOut> utxo = nullptr; //TODO: this could be a ref
        if (!UnspentTxOut::Set.contains(txIn->ToSpend))
        {
            utxo = UnspentTxOut::Set[txIn->ToSpend];
        }
        else
        {
            if (!req.SiblingsInBlock.empty())
            {
                utxo = UnspentTxOut::FindInList(txIn, req.SiblingsInBlock);
            }

            if (req.Allow_UTXO_FromMempool)
            {
                //TODO: find utxo in mempool
            }
        }

        if (utxo == nullptr)
            throw std::exception("Tx::Validate --- utxo == nullptr");

        if (utxo->IsCoinbase && (Chain::GetCurrentHeight() - utxo->Height) < NetParams::COINBASE_MATURITY)
            throw std::exception("Tx::Validate --- utxo->IsCoinbase && (Chain::GetCurrentHeight() - utxo->Height) < NetParams::COINBASE_MATURITY");

        //TODO: verify signature

        avaliableToSpend += utxo->TxOut->Value;
    }

    uint64_t totalSpent = 0;
    for (const auto& txOut : tx->TxOuts)
        totalSpent += txOut->Value;

    if (avaliableToSpend < totalSpent)
        throw std::exception("Tx::Validate --- avaliableToSpend < totalSpent");
}
