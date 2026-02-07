#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "util/binary_buffer.hpp"
#include "core/block.hpp"
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

TEST(BlockSerializationTest, RoundTrip)
{
	auto to_spend = std::make_shared<TxOutPoint>("deadbeef", 0);
	auto tx_in = std::make_shared<TxIn>(to_spend, std::vector<uint8_t>{0x01, 0x02}, std::vector<uint8_t>{0x03}, -1);
	auto tx_out = std::make_shared<TxOut>(5000000000ULL, "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs");
	auto tx = std::make_shared<Tx>(std::vector{ tx_in }, std::vector{ tx_out }, 0);

	auto to_spend2 = std::make_shared<TxOutPoint>("cafebabe", 1);
	auto tx_in2 = std::make_shared<TxIn>(to_spend2, std::vector<uint8_t>{0xAA}, std::vector<uint8_t>{0xBB, 0xCC}, 42);
	auto tx_out2 = std::make_shared<TxOut>(100, "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa");
	auto tx2 = std::make_shared<Tx>(std::vector{ tx_in2 }, std::vector{ tx_out2 }, 500);

	Block original(1, "prev_hash_abc", "merkle_hash_xyz", 1609459200, 24, 12345,
		std::vector<std::shared_ptr<Tx>>{tx, tx2});

	auto serialized = original.serialize();

	Block deserialized;
	ASSERT_TRUE(deserialized.deserialize(serialized));

	EXPECT_EQ(original.version, deserialized.version);
	EXPECT_EQ(original.prev_block_hash, deserialized.prev_block_hash);
	EXPECT_EQ(original.merkle_hash, deserialized.merkle_hash);
	EXPECT_EQ(original.timestamp, deserialized.timestamp);
	EXPECT_EQ(original.bits, deserialized.bits);
	EXPECT_EQ(original.nonce, deserialized.nonce);
	ASSERT_EQ(original.txs.size(), deserialized.txs.size());
	for (size_t i = 0; i < original.txs.size(); i++)
	{
		EXPECT_EQ(*original.txs[i], *deserialized.txs[i]);
	}
	EXPECT_EQ(original, deserialized);
	EXPECT_EQ(original.id(), deserialized.id());
}

TEST(BlockSerializationTest, EmptyBlockRoundTrip)
{
	Block original(0, "", "", 0, 0, 0, std::vector<std::shared_ptr<Tx>>{});

	auto serialized = original.serialize();

	Block deserialized;
	ASSERT_TRUE(deserialized.deserialize(serialized));

	EXPECT_EQ(original, deserialized);
	EXPECT_EQ(original.id(), deserialized.id());
}

TEST(BlockSerializationTest, DeserializeTruncatedData)
{
	auto tx_out = std::make_shared<TxOut>(100, "addr");
	auto tx_in = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{}, std::vector<uint8_t>{}, -1);
	auto tx = std::make_shared<Tx>(std::vector{ tx_in }, std::vector{ tx_out }, 0);
	Block original(1, "prev", "merkle", 100, 24, 999, std::vector<std::shared_ptr<Tx>>{tx});

	auto serialized = original.serialize();
	auto& buf = serialized.get_writable_buffer();

	std::vector<uint8_t> truncated(buf.begin(), buf.begin() + buf.size() / 2);
	BinaryBuffer truncated_buf(std::move(truncated));

	Block deserialized;
	EXPECT_FALSE(deserialized.deserialize(truncated_buf));
}

TEST(BlockSerializationTest, HeaderPrefixPlusNonceEqualsHeader)
{
	Block block(1, "prev_hash", "merkle_hash", 1609459200, 24, 42,
		std::vector<std::shared_ptr<Tx>>{});

	auto prefix = block.header_prefix();
	BinaryBuffer prefix_with_nonce(prefix.get_buffer());
	prefix_with_nonce.write(block.nonce);

	auto full_header = block.header();
	EXPECT_EQ(prefix_with_nonce.get_buffer(), full_header.get_buffer());

	const uint64_t override_nonce = 9999;
	auto header_overridden = block.header(override_nonce);
	BinaryBuffer prefix_with_override(prefix.get_buffer());
	prefix_with_override.write(override_nonce);
	EXPECT_EQ(prefix_with_override.get_buffer(), header_overridden.get_buffer());
}

TEST(BlockSerializationTest, CopyAndMoveProduceCorrectId)
{
	auto tx_out = std::make_shared<TxOut>(100, "addr");
	auto tx_in = std::make_shared<TxIn>(nullptr, std::vector<uint8_t>{}, std::vector<uint8_t>{}, -1);
	auto tx = std::make_shared<Tx>(std::vector{ tx_in }, std::vector{ tx_out }, 0);
	Block original(1, "prev", "merkle", 100, 24, 42, std::vector<std::shared_ptr<Tx>>{tx});
	const auto original_id = original.id();

	Block copied(original);
	EXPECT_EQ(original_id, copied.id());
	EXPECT_EQ(original, copied);

	Block assigned;
	assigned = original;
	EXPECT_EQ(original_id, assigned.id());

	Block to_move(original);
	Block moved(std::move(to_move));
	EXPECT_EQ(original_id, moved.id());

	Block to_move2(original);
	Block move_assigned;
	move_assigned = std::move(to_move2);
	EXPECT_EQ(original_id, move_assigned.id());
}
