#include "pch.hpp"

#include <cstdlib>

#include "../tiny-lib/Log.hpp"
#include "gtest/gtest.h"

void atexit_handler()
{
	Log::StopLog();
}

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);

	Log::StartLog(false);
	if (std::atexit(atexit_handler) != 0)
		return EXIT_FAILURE;

	return RUN_ALL_TESTS();
}
