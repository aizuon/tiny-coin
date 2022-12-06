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

	EXPECT_TRUE(tx2->TxIns.size() == 1);
	auto& tx_in2 = tx2->TxIns[0];
	EXPECT_EQ(tx_in2->Sequence, tx_in->Sequence);
	auto& to_spend2 = tx_in2->ToSpend;
	EXPECT_EQ(to_spend2->TxId, to_spend->TxId);
	EXPECT_EQ(to_spend2->TxOutIdx, to_spend->TxOutIdx);
	EXPECT_EQ(tx_in2->UnlockPubKey, tx_in->UnlockPubKey);
	EXPECT_EQ(tx_in2->UnlockSig, tx_in->UnlockSig);

	EXPECT_TRUE(tx2->TxOuts.size() == 1);
	auto& tx_out2 = tx2->TxOuts[0];
	EXPECT_EQ(tx_out2->ToAddress, tx_out->ToAddress);
	EXPECT_EQ(tx_out2->Value, tx_out->Value);

	EXPECT_EQ(tx2->LockTime, tx->LockTime);
}

TEST(SerializationTest, TxInSerialization)
{
	auto to_spend = std::make_shared<TxOutPoint>("foo", 0);
	const auto tx_in = std::make_shared<TxIn>(to_spend, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);

	auto serialized_buffer = tx_in->Serialize();

	const auto tx_in2 = std::make_shared<TxIn>();
	ASSERT_TRUE(tx_in2->Deserialize(serialized_buffer));

	EXPECT_EQ(tx_in2->Sequence, tx_in->Sequence);

	const auto& to_spend2 = tx_in2->ToSpend;
	EXPECT_EQ(to_spend2->TxId, to_spend->TxId);
	EXPECT_EQ(to_spend2->TxOutIdx, to_spend->TxOutIdx);

	EXPECT_EQ(tx_in2->UnlockPubKey, tx_in->UnlockPubKey);
	EXPECT_EQ(tx_in2->UnlockSig, tx_in->UnlockSig);
}

TEST(SerializationTest, TxOutSerialization)
{
	const auto tx_out = std::make_shared<TxOut>(0, "foo");

	auto serialized_buffer = tx_out->Serialize();

	const auto tx_out2 = std::make_shared<TxOut>();
	ASSERT_TRUE(tx_out2->Deserialize(serialized_buffer));

	EXPECT_EQ(tx_out2->ToAddress, tx_out->ToAddress);
	EXPECT_EQ(tx_out2->Value, tx_out->Value);
}

TEST(SerializationTest, TxOutPointSerialization)
{
	const auto tx_out_point = std::make_shared<TxOutPoint>("foo", 0);

	auto serialized_buffer = tx_out_point->Serialize();

	const auto tx_out_point2 = std::make_shared<TxOutPoint>();
	ASSERT_TRUE(tx_out_point2->Deserialize(serialized_buffer));

	EXPECT_EQ(tx_out_point2->TxId, tx_out_point->TxId);
	EXPECT_EQ(tx_out_point2->TxOutIdx, tx_out_point->TxOutIdx);
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
	EXPECT_EQ(tx_out2->ToAddress, tx_out->ToAddress);
	EXPECT_EQ(tx_out2->Value, tx_out->Value);

	const auto& tx_out_point2 = utxo->TxOutPoint;
	EXPECT_EQ(tx_out_point2->TxId, tx_out_point->TxId);
	EXPECT_EQ(tx_out_point2->TxOutIdx, tx_out_point->TxOutIdx);

	EXPECT_EQ(utxo2->IsCoinbase, utxo->IsCoinbase);
	EXPECT_EQ(utxo2->Height, utxo->Height);
}
