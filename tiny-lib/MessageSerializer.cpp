#include "pch.hpp"

#include "MessageSerializer.hpp"
#include "TxOut.hpp"
#include "TxOutPoint.hpp"
#include "BinaryBuffer.hpp"
#include "SHA256.hpp"

std::vector<uint8_t> MessageSerializer::BuildSpendMessage(const std::shared_ptr<TxOutPoint>& toSpend, const std::vector<uint8_t>& pubKey, int32_t sequence, const std::vector<std::shared_ptr<TxOut>>& txOuts)
{
    BinaryBuffer spendMessage;
    spendMessage.Write(toSpend->Serialize());
    spendMessage.Write(sequence);
    spendMessage.Write(pubKey);
    for (const auto& txOut : txOuts)
    {
        spendMessage.Write(txOut->Serialize());
    }

    auto buffer = spendMessage.GetBuffer();

    auto hash = SHA256::DoubleHashBinary(buffer);

    return hash;
}
