#include "crypto/base58.hpp"

#include <algorithm>
#include <openssl/bn.h>

std::string Base58::encode(const std::vector<uint8_t>& buffer)
{
	auto bn_ctx = BN_CTX_new();
	auto bn = BN_new();
	auto bn00 = BN_new();
	auto bn58 = BN_new();
	auto dv = BN_new();
	auto rem = BN_new();

	auto cleanup = [&]()
	{
		BN_free(rem);
		BN_free(dv);
		BN_free(bn58);
		BN_free(bn00);
		BN_free(bn);
		BN_CTX_free(bn_ctx);
	};

	if (!BN_bin2bn(buffer.data(), static_cast<int>(buffer.size()), bn)
		|| !BN_set_word(bn58, 58))
	{
		cleanup();
		return {};
	}
	BN_zero(bn00);

	std::string result;
	while (BN_cmp(bn, bn00) > 0)
	{
		if (!BN_div(dv, rem, bn, bn58, bn_ctx) || BN_copy(bn, dv) == nullptr)
		{
			cleanup();
			return {};
		}
		const auto base58char = table[BN_get_word(rem)];
		result += base58char;
	}

	cleanup();

	for (const auto& byte : buffer)
	{
		if (byte != 0x00)
			break;
		result += '1';
	}

	std::ranges::reverse(result);

	return result;
}
