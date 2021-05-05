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
	std::vector<std::shared_ptr<TxIn>> txIns;
	auto toSpend = std::make_shared<TxOutPoint>("foo", 0);
	const auto txIn = std::make_shared<TxIn>(toSpend, std::vector<uint8_t>{0x00}, std::vector<uint8_t>{0x00}, 1);
	txIns.push_back(txIn);

	std::vector<std::shared_ptr<TxOut>> txOuts;
	const auto txOut = std::make_shared<TxOut>(0, "foo");
	txOuts.push_back(txOut);

	auto tx = std::make_shared<Tx>(txIns, txOuts, -1);

	const auto spendMsg = MsgSerializer::BuildSpendMsg(txIn->ToSpend, txIn->UnlockPubKey, txIn->Sequence, tx->TxOuts);
	auto spendMsg_str = Utils::ByteArrayToHexString(spendMsg);

	EXPECT_EQ(spendMsg_str, "e27095c3fbcceb55b0da735f9c897e9ea58a123c3508e55d60c346457e513ba6");

	const auto txOut2 = std::make_shared<TxOut>(0, "foo");
	tx->TxOuts.push_back(txOut2);

	const auto spendMsg2 = MsgSerializer::BuildSpendMsg(txIn->ToSpend, txIn->UnlockPubKey, txIn->Sequence, tx->TxOuts);
	const auto spendMsg2_str = Utils::ByteArrayToHexString(spendMsg2);

	EXPECT_NE(spendMsg2_str, spendMsg_str);
}
