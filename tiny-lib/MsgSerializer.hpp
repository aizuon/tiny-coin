#pragma once
#include <cstdint>
#include <vector>
#include <memory>

class TxOut;
class TxOutPoint;

class MsgSerializer
{
public:
	static std::vector<uint8_t> BuildSpendMsg(const std::shared_ptr<TxOutPoint>& toSpend, const std::vector<uint8_t>& pubKey, int32_t sequence, const std::vector<std::shared_ptr<TxOut>>& txOuts);
};