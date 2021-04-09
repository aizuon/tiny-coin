#include "pch.h"

#include <fmt/format.h>

#include "Transaction.h"

bool Transaction::IsCoinbase()
{
    return TxIns->size() == 1 && (*TxIns)[0]->ToSpend == nullptr;
}

std::shared_ptr<Transaction> Transaction::CreateCoinbase(const std::string& PayToAddr, uint64_t value, int64_t height)
{
    auto tx_in = std::make_shared<TxIn>(nullptr, std::make_shared<std::vector<uint8_t>>(sizeof(height)), nullptr, 0);
    memcpy(tx_in->UnlockSig->data(), &height, sizeof(height));

    auto tx_out = std::make_shared<TxOut>(value, PayToAddr);

    auto tx = std::make_shared<Transaction>(std::make_shared<std::vector<std::shared_ptr<TxIn>>>(std::initializer_list<std::shared_ptr<TxIn>>{ tx_in }), std::make_shared<std::vector<std::shared_ptr<TxOut>>>(std::initializer_list<std::shared_ptr<TxOut>>{ tx_out }), -1);

    return tx;
}
