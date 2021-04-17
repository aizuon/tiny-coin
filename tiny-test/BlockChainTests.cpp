#include "pch.hpp"

#include <cstdint>
#include <array>
#include <vector>
#include <memory>

#include "gtest/gtest.h"

#include "../tiny-lib/Block.hpp"
#include "../tiny-lib/Chain.hpp"

TEST(BlockChainTest, MedianTimePast)
{
	Chain::ActiveChain = std::vector<std::shared_ptr<Block>>();

	EXPECT_TRUE(Chain::GetMedianTimePast(10) == 0);

	std::array<int64_t, 5> timestamps{ 1, 30, 60, 90, 400 };

	for (auto timestamp : timestamps)
	{
		auto dummyBlock = std::make_shared<Block>(0, "foo", "foo", timestamp, 1, 0, std::vector<std::shared_ptr<Tx>>());

		Chain::ActiveChain.push_back(dummyBlock);
	}

	EXPECT_TRUE(Chain::GetMedianTimePast(1), 400);
	EXPECT_TRUE(Chain::GetMedianTimePast(3), 90);
	EXPECT_TRUE(Chain::GetMedianTimePast(2), 90);
	EXPECT_TRUE(Chain::GetMedianTimePast(5), 60);
}

TEST(BlockChainTest, DependentTxsInSingleBlock)
{
	//TODO
}

TEST(BlockChainTest, Reorg)
{
	//TODO
}