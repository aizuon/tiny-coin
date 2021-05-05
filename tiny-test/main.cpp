#include "pch.hpp"

#include "../tiny-lib/Log.hpp"
#include "gtest/gtest.h"

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);

	Log::StartLog();

	return RUN_ALL_TESTS();
}
