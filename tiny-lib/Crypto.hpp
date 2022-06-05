#pragma once
#include <openssl/provider.h>

class Crypto
{
public:
	static void Init();
	static void CleanUp();

private:
	static OSSL_PROVIDER* pDefault;
	static OSSL_PROVIDER* pLegacy;
};
