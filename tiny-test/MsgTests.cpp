#include "pch.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../tiny-lib/MsgSerializer.hpp"
#include "../tiny-lib/Tx.hpp"
#include "../tiny-lib/TxIn.hpp"
#include "../tiny-lib/TxOut.hpp"
#include "../tiny-lib/TxOutPoint.hpp"
#include "../tiny-lib/Utils.hpp"
#include "gtest/gtest.h"

TEST(MsgTest, SpendMsg)
{
	std::vector<std::shared_ptr<TxIn>> tx_ins;
	auto to_spend = std::make_shared<TxOutPoint>("foo", 0);
	const auto tx_in = std::make_shared<TxIn>(to_spend, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
	tx_ins.push_back(tx_in);

	std::vector<std::shared_ptr<TxOut>> tx_outs;
	const auto tx_out = std::make_shared<TxOut>(0, "foo");
	tx_outs.push_back(tx_out);

	const auto tx = std::make_shared<Tx>(tx_ins, tx_outs, 0);

	const auto spend_msg = MsgSerializer::BuildSpendMsg(tx_in->ToSpend, tx_in->UnlockPubKey, tx_in->Sequence,
	                                                    tx->TxOuts);
	const auto spend_msg_str = Utils::ByteArrayToHexString(spend_msg);

	EXPECT_EQ(spend_msg_str, "d2cde10c62cdc1707ad78d7356e01a73d1376a7a1f775ca6d207d8a511fdff19");

	const auto tx_out2 = std::make_shared<TxOut>(0, "foo");
	tx->TxOuts.push_back(tx_out2);

	const auto spend_msg2 = MsgSerializer::BuildSpendMsg(tx_in->ToSpend, tx_in->UnlockPubKey, tx_in->Sequence,
	                                                     tx->TxOuts);
	const auto spend_msg2_str = Utils::ByteArrayToHexString(spend_msg2);

	EXPECT_NE(spend_msg2_str, spend_msg_str);
}
