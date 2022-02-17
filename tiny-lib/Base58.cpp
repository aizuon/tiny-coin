#include "pch.hpp"
#include "Base58.hpp"

#include <ranges>
#include <openssl/bn.h>

#include "Utils.hpp"

std::string Base58::Encode(const std::vector<uint8_t>& buffer)
{
	auto* bnctx = BN_CTX_new();
	auto* bn = BN_new();
	auto* bn00 = BN_new();
	auto* bn58 = BN_new();
	auto* dv = BN_new();
	auto* rem = BN_new();

	const auto hexString = Utils::ByteArrayToHexString(buffer);

	if (!BN_hex2bn(&bn, hexString.c_str()) || !BN_hex2bn(&bn58, "3a") || !BN_hex2bn(&bn00, "00"))
	{
		BN_free(rem);
		BN_free(dv);
		BN_free(bn58);
		BN_free(bn00);
		BN_free(bn);
		BN_CTX_free(bnctx);

		return {};
	}
	std::string result;
	while (BN_cmp(bn, bn00) > 0)
	{
		if (!BN_div(dv, rem, bn, bn58, bnctx) || BN_copy(bn, dv) == nullptr)
		{
			BN_free(rem);
			BN_free(dv);
			BN_free(bn58);
			BN_free(bn00);
			BN_free(bn);
			BN_CTX_free(bnctx);

			return {};
		}
		const auto base58char = Table[BN_get_word(rem)];
		result += base58char;
	}

	BN_free(rem);
	BN_free(dv);
	BN_free(bn58);
	BN_free(bn00);
	BN_free(bn);
	BN_CTX_free(bnctx);

	std::ranges::reverse(result);

	return result;
}
