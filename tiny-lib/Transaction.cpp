#include "pch.hpp"

#include <fmt/format.h>

#include "Transaction.hpp"
#include "BinaryBuffer.hpp"

bool Transaction::IsCoinbase() const
{
    return TxIns.size() == 1 && TxIns[0]->ToSpend == nullptr;
}

std::vector<uint8_t> Transaction::Serialize() const
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

std::shared_ptr<Transaction> Transaction::CreateCoinbase(const std::string& PayToAddr, uint64_t value, int64_t height)
{
    BinaryBuffer tx_in_unlockSig(sizeof(height));
    tx_in_unlockSig.Write(height);
    auto tx_in = std::make_shared<TxIn>(nullptr, tx_in_unlockSig.GetBuffer(), std::vector<uint8_t>(), -1);

    auto tx_out = std::make_shared<TxOut>(value, PayToAddr);

    auto tx_ins = std::vector<std::shared_ptr<TxIn>>{ tx_in };
    auto tx_outs = std::vector<std::shared_ptr<TxOut>>{ tx_out };
    auto tx = std::make_shared<Transaction>(tx_ins, tx_outs, -1);

    return tx;
}
