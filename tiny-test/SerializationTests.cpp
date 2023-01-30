#include "pch.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../tiny-lib/BinaryBuffer.hpp"
#include "../tiny-lib/Tx.hpp"
#include "../tiny-lib/TxIn.hpp"
#include "../tiny-lib/TxOut.hpp"
#include "../tiny-lib/TxOutPoint.hpp"
#include "../tiny-lib/UnspentTxOut.hpp"
#include "gtest/gtest.h"

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

	auto serialized_buffer = tx->Serialize();

	auto tx2 = std::make_shared<Tx>();
	ASSERT_TRUE(tx2->Deserialize(serialized_buffer));

	EXPECT_EQ(1, tx2->TxIns.size());
	auto& tx_in2 = tx2->TxIns[0];
	EXPECT_EQ(tx_in->Sequence, tx_in2->Sequence);
	auto& to_spend2 = tx_in2->ToSpend;
	EXPECT_EQ(to_spend->TxId, to_spend2->TxId);
	EXPECT_EQ(to_spend->TxOutIdx, to_spend2->TxOutIdx);
	EXPECT_EQ(tx_in->UnlockPubKey, tx_in2->UnlockPubKey);
	EXPECT_EQ(tx_in->UnlockSig, tx_in2->UnlockSig);

	EXPECT_EQ(1, tx2->TxOuts.size());
	auto& tx_out2 = tx2->TxOuts[0];
	EXPECT_EQ(tx_out->ToAddress, tx_out2->ToAddress);
	EXPECT_EQ(tx_out->Value, tx_out2->Value);

	EXPECT_EQ(tx->LockTime, tx2->LockTime);
}

TEST(SerializationTest, TxInSerialization)
{
	auto to_spend = std::make_shared<TxOutPoint>("foo", 0);
	const auto tx_in = std::make_shared<TxIn>(to_spend, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);

	auto serialized_buffer = tx_in->Serialize();

	const auto tx_in2 = std::make_shared<TxIn>();
	ASSERT_TRUE(tx_in2->Deserialize(serialized_buffer));

	EXPECT_EQ(tx_in->Sequence, tx_in2->Sequence);

	const auto& to_spend2 = tx_in2->ToSpend;
	EXPECT_EQ(to_spend->TxId, to_spend2->TxId);
	EXPECT_EQ(to_spend->TxOutIdx, to_spend2->TxOutIdx);

	EXPECT_EQ(tx_in->UnlockPubKey, tx_in2->UnlockPubKey);
	EXPECT_EQ(tx_in->UnlockSig, tx_in2->UnlockSig);
}

TEST(SerializationTest, TxOutSerialization)
{
	const auto tx_out = std::make_shared<TxOut>(0, "foo");

	auto serialized_buffer = tx_out->Serialize();

	const auto tx_out2 = std::make_shared<TxOut>();
	ASSERT_TRUE(tx_out2->Deserialize(serialized_buffer));

	EXPECT_EQ(tx_out->ToAddress, tx_out2->ToAddress);
	EXPECT_EQ(tx_out->Value, tx_out2->Value);
}

TEST(SerializationTest, TxOutPointSerialization)
{
	const auto tx_out_point = std::make_shared<TxOutPoint>("foo", 0);

	auto serialized_buffer = tx_out_point->Serialize();

	const auto tx_out_point2 = std::make_shared<TxOutPoint>();
	ASSERT_TRUE(tx_out_point2->Deserialize(serialized_buffer));

	EXPECT_EQ(tx_out_point->TxId, tx_out_point2->TxId);
	EXPECT_EQ(tx_out_point->TxOutIdx, tx_out_point2->TxOutIdx);
}

TEST(SerializationTest, UnspentTxOutSerialization)
{
	auto tx_out = std::make_shared<TxOut>(0, "foo");

	auto tx_out_point = std::make_shared<TxOutPoint>("foo", 0);

	const auto utxo = std::make_shared<UTXO>(tx_out, tx_out_point, false, 0);

	auto serialized_buffer = utxo->Serialize();

	const auto utxo2 = std::make_shared<UTXO>();
	ASSERT_TRUE(utxo2->Deserialize(serialized_buffer));

	const auto& tx_out2 = utxo->TxOut;
	EXPECT_EQ(tx_out->ToAddress, tx_out2->ToAddress);
	EXPECT_EQ(tx_out->Value, tx_out2->Value);

	const auto& tx_out_point2 = utxo->TxOutPoint;
	EXPECT_EQ(tx_out_point->TxId, tx_out_point2->TxId);
	EXPECT_EQ(tx_out_point->TxOutIdx, tx_out_point2->TxOutIdx);

	EXPECT_EQ(utxo->IsCoinbase, utxo2->IsCoinbase);
	EXPECT_EQ(utxo->Height, utxo2->Height);
}
