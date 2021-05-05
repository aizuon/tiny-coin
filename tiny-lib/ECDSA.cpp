#include "pch.hpp"

#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>

#include "ECDSA.hpp"

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> ECDSA::Generate()
{
	EC_KEY* ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
	if (!EC_KEY_generate_key(ec_key))
	{
		EC_KEY_free(ec_key);

		return {std::vector<uint8_t>(), std::vector<uint8_t>()};
	}

	const BIGNUM* priv_key = EC_KEY_get0_private_key(ec_key);
	std::vector<uint8_t> priv_key_buffer(BN_num_bytes(priv_key));
	if (!BN_bn2bin(priv_key, priv_key_buffer.data()))
	{
		EC_KEY_free(ec_key);

		return {std::vector<uint8_t>(), std::vector<uint8_t>()};
	}

	const EC_GROUP* ec_group = EC_KEY_get0_group(ec_key);
	auto pub_key_buffer = GetPubKeyFromPrivKey(ec_key, ec_group);
	if (pub_key_buffer.size() == 0)
	{
		EC_KEY_free(ec_key);

		return {std::vector<uint8_t>(), std::vector<uint8_t>()};
	}

	EC_KEY_free(ec_key);

	return {priv_key_buffer, pub_key_buffer};
}

std::vector<uint8_t> ECDSA::GetPubKeyFromPrivKey(const std::vector<uint8_t>& privKey)
{
	EC_KEY* ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);

	if (!ImportPrivKey(ec_key, privKey))
	{
		EC_KEY_free(ec_key);

		return std::vector<uint8_t>();
	}

	const EC_GROUP* ec_group = EC_KEY_get0_group(ec_key);
	auto pub_key_buffer = GetPubKeyFromPrivKey(ec_key, ec_group);
	if (pub_key_buffer.size() == 0)
	{
		EC_KEY_free(ec_key);

		return std::vector<uint8_t>();
	}

	EC_KEY_free(ec_key);

	return pub_key_buffer;
}

std::vector<uint8_t> ECDSA::SignMsg(const std::vector<uint8_t>& msg, const std::vector<uint8_t>& privKey)
{
	EC_KEY* ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);

	if (!ImportPrivKey(ec_key, privKey))
	{
		EC_KEY_free(ec_key);

		return std::vector<uint8_t>();
	}

	std::vector<uint8_t> sig(ECDSA_size(ec_key));
	unsigned int siglen = sig.size();
	if (!ECDSA_sign(0, msg.data(), msg.size(), sig.data(), &siglen, ec_key))
	{
		EC_KEY_free(ec_key);

		return std::vector<uint8_t>();
	}
	sig.resize(siglen);

	EC_KEY_free(ec_key);

	return sig;
}

bool ECDSA::VerifySig(const std::vector<uint8_t>& sig, const std::vector<uint8_t>& msg,
                      const std::vector<uint8_t>& pubKey)
{
	EC_KEY* ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);

	BIGNUM* pub_key_bn = BN_new();
	if (BN_bin2bn(pubKey.data(), pubKey.size(), pub_key_bn) == NULL)
	{
		BN_free(pub_key_bn);

		EC_KEY_free(ec_key);

		return false;
	}

	const EC_GROUP* ec_group = EC_KEY_get0_group(ec_key);
	EC_POINT* pub_key_point = EC_POINT_new(ec_group);
	BN_CTX* bn_ctx = BN_CTX_new();
	if (EC_POINT_bn2point(ec_group, pub_key_bn, pub_key_point, bn_ctx) == NULL)
	{
		BN_CTX_free(bn_ctx);

		EC_POINT_free(pub_key_point);

		BN_free(pub_key_bn);

		EC_KEY_free(ec_key);

		return false;
	}
	BN_CTX_free(bn_ctx);
	BN_free(pub_key_bn);

	if (!EC_KEY_set_public_key(ec_key, pub_key_point))
	{
		EC_POINT_free(pub_key_point);

		EC_KEY_free(ec_key);

		return false;
	}

	if (!ECDSA_verify(0, msg.data(), msg.size(), sig.data(), sig.size(), ec_key))
	{
		EC_POINT_free(pub_key_point);

		EC_KEY_free(ec_key);

		return false;
	}

	EC_POINT_free(pub_key_point);

	EC_KEY_free(ec_key);

	return true;
}

bool ECDSA::ImportPrivKey(EC_KEY* ec_key, const std::vector<uint8_t>& privKey)
{
	BIGNUM* priv_key_bn = BN_new();
	if (BN_bin2bn(privKey.data(), privKey.size(), priv_key_bn) == NULL)
	{
		BN_free(priv_key_bn);

		return false;
	}

	if (!EC_KEY_set_private_key(ec_key, priv_key_bn))
	{
		return false;
	}
	BN_free(priv_key_bn);

	return true;
}

std::vector<uint8_t> ECDSA::GetPubKeyFromPrivKey(EC_KEY* ec_key, const EC_GROUP* ec_group)
{
	BN_CTX* bn_ctx = BN_CTX_new();

	const EC_POINT* pub_key = EC_KEY_get0_public_key(ec_key);
	if (pub_key == NULL)
	{
		EC_POINT* pub_key2 = EC_POINT_new(ec_group);
		const BIGNUM* priv_key = EC_KEY_get0_private_key(ec_key);
		if (!EC_POINT_mul(ec_group, pub_key2, priv_key, NULL, NULL, bn_ctx))
		{
			BN_CTX_free(bn_ctx);

			return std::vector<uint8_t>();
		}
		if (!EC_KEY_set_public_key(ec_key, pub_key2))
		{
			BN_CTX_free(bn_ctx);

			EC_POINT_free(pub_key2);

			return std::vector<uint8_t>();
		}
		if (!EC_KEY_check_key(ec_key))
		{
			BN_CTX_free(bn_ctx);

			EC_POINT_free(pub_key2);

			return std::vector<uint8_t>();
		}
		pub_key = pub_key2;
	}

	BIGNUM* pub_key_bn = BN_new();
	if (EC_POINT_point2bn(ec_group, pub_key, POINT_CONVERSION_COMPRESSED, pub_key_bn, bn_ctx) == NULL)
	{
		BN_free(pub_key_bn);
		BN_CTX_free(bn_ctx);

		return std::vector<uint8_t>();
	}
	std::vector<uint8_t> pub_key_buffer(BN_num_bytes(pub_key_bn));
	if (!BN_bn2bin(pub_key_bn, pub_key_buffer.data()))
	{
		BN_free(pub_key_bn);
		BN_CTX_free(bn_ctx);

		return std::vector<uint8_t>();
	}
	BN_free(pub_key_bn);
	BN_CTX_free(bn_ctx);

	return pub_key_buffer;
}
