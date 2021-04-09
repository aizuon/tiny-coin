#include "pch.hpp"

#include <fmt/format.h>

#include "Transaction.hpp"

bool Transaction::IsCoinbase() const
{
    return TxIns->size() == 1 && (*TxIns)[0]->ToSpend == nullptr;
}

std::shared_ptr<Transaction> Transaction::CreateCoinbase(const std::string& PayToAddr, uint64_t value, int64_t height)
{
    auto tx_in_unlockSig = std::make_shared<std::vector<uint8_t>>(sizeof(height));
    memcpy(tx_in_unlockSig->data(), &height, sizeof(height));
    auto tx_in = std::make_shared<TxIn>(nullptr, tx_in_unlockSig, nullptr, 0);

    auto tx_out = std::make_shared<TxOut>(value, PayToAddr);

    auto tx_ins = std::make_shared<std::vector<std::shared_ptr<TxIn>>>(std::initializer_list<std::shared_ptr<TxIn>>{ tx_in });
    auto tx_outs = std::make_shared<std::vector<std::shared_ptr<TxOut>>>(std::initializer_list<std::shared_ptr<TxOut>>{ tx_out });
    auto tx = std::make_shared<Transaction>(tx_ins, tx_outs, -1);

    return tx;
}
