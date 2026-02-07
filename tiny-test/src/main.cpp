#include <cstdlib>

#include "util/log.hpp"
#include "crypto/crypto.hpp"
#include <gtest/gtest.h>

void atexit_handler()
{
	Crypto::cleanup();
	Log::stop_log();
}

int main(int argc, char** argv)
{
	Log::start_log(false);
	Crypto::init();
	if (std::atexit(atexit_handler) != 0)
		return EXIT_FAILURE;

	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
