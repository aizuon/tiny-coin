#include "pch.hpp"

#include <exception>
#include <fmt/format.h>

#include "Tx.hpp"
#include "TxIn.hpp"
#include "TxOut.hpp"
#include "UnspentTxOut.hpp"
#include "NetParams.hpp"
#include "Utils.hpp"
#include "ECDSA.hpp"
#include "SHA256.hpp"
#include "BinaryBuffer.hpp"
#include "MessageSerializer.hpp"
#include "Chain.hpp"
#include "Mempool.hpp"
#include "Wallet.hpp"

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
    return Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Serialize()));
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

void Tx::Validate(const ValidateRequest& req)
{
    ValidateBasics(req.AsCoinbase);

    uint64_t avaliableToSpend = 0;
    for (size_t i = 0; i < TxIns.size(); i++)
    {
        const auto& txIn = TxIns[i];

        std::shared_ptr<UnspentTxOut> utxo = nullptr; //HACK: this should be a ref
        if (!UnspentTxOut::Map.contains(txIn->ToSpend))
        {
            utxo = UnspentTxOut::Map[txIn->ToSpend];
        }
        else
        {
            if (!req.SiblingsInBlock.empty())
            {
                utxo = UnspentTxOut::FindInList(txIn, req.SiblingsInBlock);
            }

            if (req.Allow_UTXO_FromMempool)
            {
                utxo = Mempool::Find_UTXO_InMempool(txIn->ToSpend);
            }
        }

        if (utxo == nullptr)
            throw std::exception("Tx::Validate --- utxo == nullptr");

        if (utxo->IsCoinbase && (Chain::GetCurrentHeight() - utxo->Height) < NetParams::COINBASE_MATURITY)
            throw std::exception("Tx::Validate --- utxo->IsCoinbase && (Chain::GetCurrentHeight() - utxo->Height) < NetParams::COINBASE_MATURITY");

        try
        {
            ValidateSignatureForSpend(txIn, utxo);
        }
        catch (...)
        {
            throw std::exception("Tx::Validate --- ValidateSignatureForSpend(txIn, utxo, tx)");
        }

        avaliableToSpend += utxo->TxOut->Value;
    }

    uint64_t totalSpent = 0;
    for (const auto& txOut : TxOuts)
        totalSpent += txOut->Value;

    if (avaliableToSpend < totalSpent)
        throw std::exception("Tx::Validate --- avaliableToSpend < totalSpent");
}

void Tx::ValidateSignatureForSpend(const std::shared_ptr<TxIn>& txIn, const std::shared_ptr<UnspentTxOut>& utxo)
{
    auto pubKeyAsAddr = Wallet::PubKeyToAddress(txIn->UnlockPubKey);
    if (pubKeyAsAddr != utxo->TxOut->ToAddress)
        throw std::exception("Tx::ValidateSignatureForSpend --- pubKeyAsAddr != utxo->TxOut->ToAddress");

    auto spend_msg = MessageSerializer::BuildSpendMessage(txIn->ToSpend, txIn->UnlockPubKey, txIn->Sequence, TxOuts);
    if (!ECDSA::VerifySig(txIn->UnlockSig, spend_msg, txIn->UnlockPubKey))
        throw std::exception("Tx::ValidateSignatureForSpend --- !ECDSA::VerifySig(txIn->UnlockSig, spend_msg, txIn->UnlockPubKey)");
}
