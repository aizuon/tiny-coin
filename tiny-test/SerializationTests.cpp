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
	std::vector<std::shared_ptr<TxIn>> txIns;
	auto toSpend = std::make_shared<TxOutPoint>("foo", 0);
	const auto txIn = std::make_shared<TxIn>(toSpend, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);
	txIns.push_back(txIn);

	std::vector<std::shared_ptr<TxOut>> txOuts;
	const auto txOut = std::make_shared<TxOut>(0, "foo");
	txOuts.push_back(txOut);

	const auto tx = std::make_shared<Tx>(txIns, txOuts, 0);

	auto serializedBuffer = tx->Serialize();

	auto tx2 = std::make_shared<Tx>();
	ASSERT_TRUE(tx2->Deserialize(serializedBuffer));

	EXPECT_TRUE(tx2->TxIns.size() == 1);
	auto& txIn2 = tx2->TxIns[0];
	EXPECT_EQ(txIn2->Sequence, txIn->Sequence);
	auto& toSpend2 = txIn2->ToSpend;
	EXPECT_EQ(toSpend2->TxId, toSpend->TxId);
	EXPECT_EQ(toSpend2->TxOutIdx, toSpend->TxOutIdx);
	EXPECT_EQ(txIn2->UnlockPubKey, txIn->UnlockPubKey);
	EXPECT_EQ(txIn2->UnlockSig, txIn->UnlockSig);

	EXPECT_TRUE(tx2->TxOuts.size() == 1);
	auto& txOut2 = tx2->TxOuts[0];
	EXPECT_EQ(txOut2->ToAddress, txOut->ToAddress);
	EXPECT_EQ(txOut2->Value, txOut->Value);

	EXPECT_EQ(tx2->LockTime, tx->LockTime);
}

TEST(SerializationTest, TxInSerialization)
{
	auto toSpend = std::make_shared<TxOutPoint>("foo", 0);
	const auto txIn = std::make_shared<TxIn>(toSpend, std::vector<uint8_t>(), std::vector<uint8_t>(), -1);

	auto serializedBuffer = txIn->Serialize();

	const auto txIn2 = std::make_shared<TxIn>();
	ASSERT_TRUE(txIn2->Deserialize(serializedBuffer));

	EXPECT_EQ(txIn2->Sequence, txIn->Sequence);

	const auto& toSpend2 = txIn2->ToSpend;
	EXPECT_EQ(toSpend2->TxId, toSpend->TxId);
	EXPECT_EQ(toSpend2->TxOutIdx, toSpend->TxOutIdx);

	EXPECT_EQ(txIn2->UnlockPubKey, txIn->UnlockPubKey);
	EXPECT_EQ(txIn2->UnlockSig, txIn->UnlockSig);
}

TEST(SerializationTest, TxOutSerialization)
{
	const auto txOut = std::make_shared<TxOut>(0, "foo");

	auto serializedBuffer = txOut->Serialize();

	const auto txOut2 = std::make_shared<TxOut>();
	ASSERT_TRUE(txOut2->Deserialize(serializedBuffer));

	EXPECT_EQ(txOut2->ToAddress, txOut->ToAddress);
	EXPECT_EQ(txOut2->Value, txOut->Value);
}

TEST(SerializationTest, TxOutPointSerialization)
{
	const auto txOutPoint = std::make_shared<TxOutPoint>("foo", 0);

	auto serializedBuffer = txOutPoint->Serialize();

	const auto txOutPoint2 = std::make_shared<TxOutPoint>();
	ASSERT_TRUE(txOutPoint2->Deserialize(serializedBuffer));

	EXPECT_EQ(txOutPoint2->TxId, txOutPoint->TxId);
	EXPECT_EQ(txOutPoint2->TxOutIdx, txOutPoint->TxOutIdx);
}

TEST(SerializationTest, UnspentTxOutSerialization)
{
	auto txOut = std::make_shared<TxOut>(0, "foo");

	auto txOutPoint = std::make_shared<TxOutPoint>("foo", 0);

	const auto utxo = std::make_shared<UTXO>(txOut, txOutPoint, false, 0);

	auto serializedBuffer = utxo->Serialize();

	const auto utxo2 = std::make_shared<UTXO>();
	ASSERT_TRUE(utxo2->Deserialize(serializedBuffer));

	const auto& txOut2 = utxo->TxOut;
	EXPECT_EQ(txOut2->ToAddress, txOut->ToAddress);
	EXPECT_EQ(txOut2->Value, txOut->Value);

	const auto& txOutPoint2 = utxo->TxOutPoint;
	EXPECT_EQ(txOutPoint2->TxId, txOutPoint->TxId);
	EXPECT_EQ(txOutPoint2->TxOutIdx, txOutPoint->TxOutIdx);

	EXPECT_EQ(utxo2->IsCoinbase, utxo->IsCoinbase);
	EXPECT_EQ(utxo2->Height, utxo->Height);
}
