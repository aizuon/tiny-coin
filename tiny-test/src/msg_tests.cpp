#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "net/msg_serializer.hpp"
#include "core/tx.hpp"
#include "core/tx_in.hpp"
#include "core/tx_out.hpp"
#include "core/tx_out_point.hpp"
#include "util/utils.hpp"
#include <gtest/gtest.h>

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

	const auto spend_msg = MsgSerializer::build_spend_msg(tx_in->to_spend, tx_in->unlock_pub_key, tx_in->sequence,
		tx->tx_outs);
	const auto spend_msg_str = Utils::byte_array_to_hex_string(spend_msg);

	EXPECT_EQ("d2cde10c62cdc1707ad78d7356e01a73d1376a7a1f775ca6d207d8a511fdff19", spend_msg_str);

	const auto tx_out2 = std::make_shared<TxOut>(0, "foo");
	tx->tx_outs.push_back(tx_out2);

	const auto spend_msg2 = MsgSerializer::build_spend_msg(tx_in->to_spend, tx_in->unlock_pub_key, tx_in->sequence,
		tx->tx_outs);
	const auto spend_msg2_str = Utils::byte_array_to_hex_string(spend_msg2);

	EXPECT_NE(spend_msg_str, spend_msg2_str);
}
