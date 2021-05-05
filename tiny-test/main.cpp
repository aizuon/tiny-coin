#include "pch.hpp"

#include "gtest/gtest.h"

#include "../tiny-lib/Log.hpp"

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);

	Log::StartLog();

	return RUN_ALL_TESTS();
}
