#include "pch.hpp"

#include <exception>
#include <fmt/format.h>

#include "Tx.hpp"
#include "NetParams.hpp"
#include "BinaryBuffer.hpp"
#include "SHA256d.hpp"
#include "Utils.hpp"

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

std::shared_ptr<Tx> Tx::CreateCoinbase(const std::string& PayToAddr, uint64_t value, int32_t height)
{
    BinaryBuffer tx_in_unlockSig(sizeof(height));
    tx_in_unlockSig.Write(height);
    auto tx_in = std::make_shared<TxIn>(nullptr, tx_in_unlockSig.GetBuffer(), std::vector<uint8_t>(), -1);

    auto tx_out = std::make_shared<TxOut>(value, PayToAddr);

    auto tx_ins = std::vector<std::shared_ptr<TxIn>>{ tx_in };
    auto tx_outs = std::vector<std::shared_ptr<TxOut>>{ tx_out };
    auto tx = std::make_shared<Tx>(tx_ins, tx_outs, -1);

    return tx;
}

std::shared_ptr<Tx> Tx::Validate(const std::shared_ptr<Tx>& tx, const ValidateRequest& req)
{
    tx->ValidateBasics(req.AsCoinbase);

    //TODO: utxo validation

    return nullptr;
}
