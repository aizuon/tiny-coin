#include "crypto/base58.hpp"

#include <algorithm>
#include <openssl/bn.h>

#include "util/utils.hpp"

std::string Base58::encode(const std::vector<uint8_t>& buffer)
{
	auto bn_ctx = BN_CTX_new();
	auto bn = BN_new();
	auto bn00 = BN_new();
	auto bn58 = BN_new();
	auto dv = BN_new();
	auto rem = BN_new();

	const auto hex_string = Utils::byte_array_to_hex_string(buffer);

	if (!BN_hex2bn(&bn, hex_string.c_str()) || !BN_hex2bn(&bn58, "3a") || !BN_hex2bn(&bn00, "00"))
	{
		BN_free(rem);
		BN_free(dv);
		BN_free(bn58);
		BN_free(bn00);
		BN_free(bn);
		BN_CTX_free(bn_ctx);

		return {};
	}
	std::string result;
	while (BN_cmp(bn, bn00) > 0)
	{
		if (!BN_div(dv, rem, bn, bn58, bn_ctx) || BN_copy(bn, dv) == nullptr)
		{
			BN_free(rem);
			BN_free(dv);
			BN_free(bn58);
			BN_free(bn00);
			BN_free(bn);
			BN_CTX_free(bn_ctx);

			return {};
		}
		const auto base58char = table[BN_get_word(rem)];
		result += base58char;
	}

	BN_free(rem);
	BN_free(dv);
	BN_free(bn58);
	BN_free(bn00);
	BN_free(bn);
	BN_CTX_free(bn_ctx);

	std::ranges::reverse(result);

	return result;
}
