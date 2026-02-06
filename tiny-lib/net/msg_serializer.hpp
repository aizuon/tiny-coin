#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "core/tx_out.hpp"
#include "core/tx_out_point.hpp"

class MsgSerializer
{
public:
	static std::vector<uint8_t> build_spend_msg(const std::shared_ptr<TxOutPoint>& to_spend,
		const std::vector<uint8_t>& pub_key, int32_t sequence,
		const std::vector<std::shared_ptr<TxOut>>& tx_outs);
};
