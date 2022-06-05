#include "pch.hpp"

#include <cstdlib>

#include "../tiny-lib/Log.hpp"
#include "../tiny-lib/Crypto.hpp"
#include "gtest/gtest.h"

void atexit_handler()
{
	Crypto::CleanUp();
	Log::StopLog();
}

int main(int argc, char** argv)
{
	Log::StartLog(false);
	Crypto::Init();
	if (std::atexit(atexit_handler) != 0)
		return EXIT_FAILURE;

	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
