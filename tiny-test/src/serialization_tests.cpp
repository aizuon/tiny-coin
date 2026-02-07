#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "util/binary_buffer.hpp"
#include "core/tx.hpp"
#include "core/tx_in.hpp"
#include "core/tx_out.hpp"
#include "core/tx_out_point.hpp"
#include "core/unspent_tx_out.hpp"
#include <gtest/gtest.h>

TEST(SerializationTest, TxSerialization)
{
	std::vector<std::shared_ptr<TxIn>> tx_ins;
	auto to_spend = std::make_shared<TxOutPoint>("foo", 0);
	const auto tx_in = std::make_shared<TxIn>(to_spend, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
	tx_ins.push_back(tx_in);

	std::vector<std::shared_ptr<TxOut>> tx_outs;
	const auto tx_out = std::make_shared<TxOut>(0, "foo");
	tx_outs.push_back(tx_out);

	const auto tx = std::make_shared<Tx>(tx_ins, tx_outs, 0);

	auto serialized_buffer = tx->serialize();

	auto tx2 = std::make_shared<Tx>();
	ASSERT_TRUE(tx2->deserialize(serialized_buffer));

	EXPECT_EQ(1, tx2->tx_ins.size());
	auto& tx_in2 = tx2->tx_ins[0];
	EXPECT_EQ(tx_in->sequence, tx_in2->sequence);
	auto& to_spend2 = tx_in2->to_spend;
	EXPECT_EQ(to_spend->tx_id, to_spend2->tx_id);
	EXPECT_EQ(to_spend->tx_out_idx, to_spend2->tx_out_idx);
	EXPECT_EQ(tx_in->unlock_pub_key, tx_in2->unlock_pub_key);
	EXPECT_EQ(tx_in->unlock_sig, tx_in2->unlock_sig);

	EXPECT_EQ(1, tx2->tx_outs.size());
	auto& tx_out2 = tx2->tx_outs[0];
	EXPECT_EQ(tx_out->to_address, tx_out2->to_address);
	EXPECT_EQ(tx_out->value, tx_out2->value);

	EXPECT_EQ(tx->lock_time, tx2->lock_time);
}

TEST(SerializationTest, TxInSerialization)
{
	auto to_spend = std::make_shared<TxOutPoint>("foo", 0);
	const auto tx_in = std::make_shared<TxIn>(to_spend, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);

	auto serialized_buffer = tx_in->serialize();

	const auto tx_in2 = std::make_shared<TxIn>();
	ASSERT_TRUE(tx_in2->deserialize(serialized_buffer));

	EXPECT_EQ(tx_in->sequence, tx_in2->sequence);

	const auto& to_spend2 = tx_in2->to_spend;
	EXPECT_EQ(to_spend->tx_id, to_spend2->tx_id);
	EXPECT_EQ(to_spend->tx_out_idx, to_spend2->tx_out_idx);

	EXPECT_EQ(tx_in->unlock_pub_key, tx_in2->unlock_pub_key);
	EXPECT_EQ(tx_in->unlock_sig, tx_in2->unlock_sig);
}

TEST(SerializationTest, TxOutSerialization)
{
	const auto tx_out = std::make_shared<TxOut>(0, "foo");

	auto serialized_buffer = tx_out->serialize();

	const auto tx_out2 = std::make_shared<TxOut>();
	ASSERT_TRUE(tx_out2->deserialize(serialized_buffer));

	EXPECT_EQ(tx_out->to_address, tx_out2->to_address);
	EXPECT_EQ(tx_out->value, tx_out2->value);
}

TEST(SerializationTest, TxOutPointSerialization)
{
	const auto tx_out_point = std::make_shared<TxOutPoint>("foo", 0);

	auto serialized_buffer = tx_out_point->serialize();

	const auto tx_out_point2 = std::make_shared<TxOutPoint>();
	ASSERT_TRUE(tx_out_point2->deserialize(serialized_buffer));

	EXPECT_EQ(tx_out_point->tx_id, tx_out_point2->tx_id);
	EXPECT_EQ(tx_out_point->tx_out_idx, tx_out_point2->tx_out_idx);
}

TEST(SerializationTest, UnspentTxOutSerialization)
{
	auto tx_out = std::make_shared<TxOut>(0, "foo");

	auto tx_out_point = std::make_shared<TxOutPoint>("foo", 0);

	const auto utxo = std::make_shared<UTXO>(tx_out, tx_out_point, false, 0);

	auto serialized_buffer = utxo->serialize();

	const auto utxo2 = std::make_shared<UTXO>();
	ASSERT_TRUE(utxo2->deserialize(serialized_buffer));

	const auto& tx_out2 = utxo2->tx_out;
	EXPECT_EQ(tx_out->to_address, tx_out2->to_address);
	EXPECT_EQ(tx_out->value, tx_out2->value);

	const auto& tx_out_point2 = utxo2->tx_out_point;
	EXPECT_EQ(tx_out_point->tx_id, tx_out_point2->tx_id);
	EXPECT_EQ(tx_out_point->tx_out_idx, tx_out_point2->tx_out_idx);

	EXPECT_EQ(utxo->is_coinbase, utxo2->is_coinbase);
	EXPECT_EQ(utxo->height, utxo2->height);
}
