#include "pch.hpp"

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "gtest/gtest.h"

#include "../tiny-lib/MsgSerializer.hpp"
#include "../tiny-lib/Tx.hpp"
#include "../tiny-lib/TxIn.hpp"
#include "../tiny-lib/TxOut.hpp"
#include "../tiny-lib/TxOutPoint.hpp"
#include "../tiny-lib/Utils.hpp"

TEST(MsgTest, SpendMsg)
{
	std::vector<std::shared_ptr<TxIn>> txIns;
	auto toSpend = std::make_shared<TxOutPoint>("foo", 0);
	auto txIn = std::make_shared<TxIn>(toSpend, std::vector <uint8_t>{ 0x00 }, std::vector <uint8_t>{ 0x00 }, 1);
	txIns.push_back(txIn);

	std::vector<std::shared_ptr<TxOut>> txOuts;
	auto txOut = std::make_shared<TxOut>(0, "foo");
	txOuts.push_back(txOut);

	auto tx = std::make_shared<Tx>(txIns, txOuts, 0);

	auto spendMsg = MsgSerializer::BuildSpendMsg(txIn->ToSpend, txIn->UnlockPubKey, txIn->Sequence, tx->TxOuts);
	auto spendMsg_str = Utils::ByteArrayToHexString(spendMsg);

	EXPECT_EQ(spendMsg_str, "b67cdaca8720b583489bde0a8ea45ae83cd5f6b2623fbf182ddbce88429b744b");

	auto txOut2 = std::make_shared<TxOut>(0, "foo");
	tx->TxOuts.push_back(txOut2);

	auto spendMsg2 = MsgSerializer::BuildSpendMsg(txIn->ToSpend, txIn->UnlockPubKey, txIn->Sequence, tx->TxOuts);
	auto spendMsg2_str = Utils::ByteArrayToHexString(spendMsg2);

	EXPECT_NE(spendMsg2_str, spendMsg_str);
}