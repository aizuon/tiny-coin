#pragma once
#include <openssl/provider.h>

class Crypto
{
public:
	static void init();
	static void cleanup();

private:
	static OSSL_PROVIDER* p_default;
	static OSSL_PROVIDER* p_legacy;
};
