#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "TxOut.hpp"
#include "TxOutPoint.hpp"

class MsgSerializer
{
public:
	static std::vector<uint8_t> BuildSpendMsg(std::shared_ptr<TxOutPoint> to_spend,
	                                          const std::vector<uint8_t>& pub_key, int32_t sequence,
	                                          const std::vector<std::shared_ptr<TxOut>>& tx_outs);
};
