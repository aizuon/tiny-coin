#include "pch.hpp"
#include "ECDSA.hpp"

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/core_names.h>

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> ECDSA::Generate()
{
	EVP_PKEY* ec_key = nullptr;

	EVP_PKEY_CTX* p_ctx = EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr);
	if (p_ctx == nullptr)
	{
		return {};
	}
	EVP_PKEY_keygen_init(p_ctx);

	auto param_bld = CreateParamBuild();
	if (param_bld == nullptr)
	{
		return {};
	}

	auto params = OSSL_PARAM_BLD_to_param(param_bld);
	OSSL_PARAM_BLD_free(param_bld);
	if (params == nullptr)
	{
		return {};
	}

	if (!EVP_PKEY_CTX_set_params(p_ctx, params))
	{
		OSSL_PARAM_free(params);

		return {};
	}
	OSSL_PARAM_free(params);

	EVP_PKEY_generate(p_ctx, &ec_key);
	if (ec_key == nullptr)
	{
		return { {}, {} };
	}

	BIGNUM* priv_key = nullptr;
	EVP_PKEY_get_bn_param(ec_key, OSSL_PKEY_PARAM_PRIV_KEY, &priv_key);
	if (priv_key == nullptr)
	{
		EVP_PKEY_free(ec_key);
		EVP_PKEY_CTX_free(p_ctx);

		return { {}, {} };
	}

	std::vector<uint8_t> priv_key_buffer(BN_num_bytes(priv_key));
	if (!BN_bn2bin(priv_key, priv_key_buffer.data()))
	{
		EVP_PKEY_free(ec_key);
		EVP_PKEY_CTX_free(p_ctx);

		return { {}, {} };
	}

	auto pub_key_buffer = GetPubKey(ec_key);
	if (pub_key_buffer.empty())
	{
		EVP_PKEY_free(ec_key);
		EVP_PKEY_CTX_free(p_ctx);

		return { {}, {} };
	}

	EVP_PKEY_free(ec_key);
	EVP_PKEY_CTX_free(p_ctx);

	return { priv_key_buffer, pub_key_buffer };
}

std::vector<uint8_t> ECDSA::GetPubKeyFromPrivKey(const std::vector<uint8_t>& priv_key)
{
	auto ec_key = CreateKey(priv_key, true);
	if (ec_key == nullptr)
	{
		return {};
	}

	auto p_ctx = EVP_PKEY_CTX_new_from_pkey(nullptr, ec_key, nullptr);
	if (p_ctx == nullptr)
	{
		EVP_PKEY_free(ec_key);

		return {};
	}

	auto pub_key_buffer = GetPubKey(ec_key);
	if (pub_key_buffer.empty())
	{
		EVP_PKEY_free(ec_key);
		EVP_PKEY_CTX_free(p_ctx);

		return {};
	}

	EVP_PKEY_free(ec_key);
	EVP_PKEY_CTX_free(p_ctx);

	return pub_key_buffer;
}

std::vector<uint8_t> ECDSA::SignMsg(const std::vector<uint8_t>& msg, const std::vector<uint8_t>& priv_key)
{
	auto ec_key = CreateKey(priv_key, true);
	if (ec_key == nullptr)
	{
		return {};
	}

	auto p_ctx = EVP_PKEY_CTX_new_from_pkey(nullptr, ec_key, nullptr);
	if (p_ctx == nullptr)
	{
		EVP_PKEY_free(ec_key);

		return {};
	}

	if (!EVP_PKEY_sign_init(p_ctx))
	{
		EVP_PKEY_free(ec_key);
		EVP_PKEY_CTX_free(p_ctx);

		return {};
	}
	size_t sig_size = 0;
	EVP_PKEY_sign(p_ctx, nullptr, &sig_size, msg.data(), msg.size());
	std::vector<uint8_t> sig(sig_size);
	if (!EVP_PKEY_sign(p_ctx, sig.data(), &sig_size, msg.data(), msg.size()))
	{
		EVP_PKEY_free(ec_key);
		EVP_PKEY_CTX_free(p_ctx);

		return {};
	}
	sig.resize(sig_size);

	EVP_PKEY_free(ec_key);
	EVP_PKEY_CTX_free(p_ctx);

	return sig;
}

bool ECDSA::VerifySig(const std::vector<uint8_t>& sig, const std::vector<uint8_t>& msg,
                      const std::vector<uint8_t>& pub_key)
{
	auto ec_key = CreateKey(pub_key);
	if (ec_key == nullptr)
	{
		return {};
	}

	auto p_ctx = EVP_PKEY_CTX_new_from_pkey(nullptr, ec_key, nullptr);
	if (p_ctx == nullptr)
	{
		EVP_PKEY_free(ec_key);

		return {};
	}

	if (!EVP_PKEY_verify_init(p_ctx))
	{
		EVP_PKEY_free(ec_key);
		EVP_PKEY_CTX_free(p_ctx);

		return {};
	}
	if (!EVP_PKEY_verify(p_ctx, sig.data(), sig.size(), msg.data(), msg.size()))
	{
		EVP_PKEY_free(ec_key);
		EVP_PKEY_CTX_free(p_ctx);

		return {};
	}

	EVP_PKEY_free(ec_key);
	EVP_PKEY_CTX_free(p_ctx);

	return true;
}

OSSL_PARAM_BLD* ECDSA::CreateParamBuild()
{
	auto param_bld = OSSL_PARAM_BLD_new();
	if (param_bld == nullptr)
	{
		return nullptr;
	}
	OSSL_PARAM_BLD_push_utf8_string(param_bld, OSSL_PKEY_PARAM_GROUP_NAME, SN_secp256k1, sizeof(SN_secp256k1));
	OSSL_PARAM_BLD_push_utf8_string(param_bld, OSSL_PKEY_PARAM_EC_POINT_CONVERSION_FORMAT,
	                                OSSL_PKEY_EC_POINT_CONVERSION_FORMAT_COMPRESSED,
	                                sizeof(OSSL_PKEY_EC_POINT_CONVERSION_FORMAT_COMPRESSED));
	return param_bld;
}

bool ECDSA::AddPrivKeyParam(OSSL_PARAM_BLD* param_bld, const std::vector<uint8_t>& priv_key)
{
	BIGNUM* priv_key_bn = BN_bin2bn(priv_key.data(), priv_key.size(), nullptr);
	if (priv_key_bn == nullptr)
	{
		return false;
	}

	return OSSL_PARAM_BLD_push_BN(param_bld, OSSL_PKEY_PARAM_PRIV_KEY, priv_key_bn);
}

bool ECDSA::AddPubKeyParam(OSSL_PARAM_BLD* param_bld, const std::vector<uint8_t>& pub_key)
{
	return OSSL_PARAM_BLD_push_octet_string(param_bld, OSSL_PKEY_PARAM_ENCODED_PUBLIC_KEY, pub_key.data(),
	                                        pub_key.size());
}

EVP_PKEY* ECDSA::CreateKey(const std::vector<uint8_t>& key, bool priv /*= false*/)
{
	EVP_PKEY* ec_key = nullptr;

	EVP_PKEY_CTX* p_ctx = EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr);
	if (p_ctx == nullptr)
	{
		return nullptr;
	}
	EVP_PKEY_fromdata_init(p_ctx);

	auto param_bld = CreateParamBuild();
	if (param_bld == nullptr)
	{
		return nullptr;
	}
	auto add_param_func = priv ? AddPrivKeyParam : AddPubKeyParam;
	if (!add_param_func(param_bld, key))
	{
		OSSL_PARAM_BLD_free(param_bld);

		return nullptr;
	}

	auto params = OSSL_PARAM_BLD_to_param(param_bld);
	OSSL_PARAM_BLD_free(param_bld);
	if (params == nullptr)
	{
		return nullptr;
	}

	EVP_PKEY_fromdata(p_ctx, &ec_key, EVP_PKEY_KEYPAIR, params);
	OSSL_PARAM_free(params);
	if (ec_key == nullptr)
	{
		EVP_PKEY_CTX_free(p_ctx);

		return nullptr;
	}

	EVP_PKEY_CTX_free(p_ctx);

	return ec_key;
}

std::vector<uint8_t> ECDSA::GetPubKey(EVP_PKEY* ec_key)
{
	size_t pub_key_size = 0;
	EVP_PKEY_get_octet_string_param(ec_key, OSSL_PKEY_PARAM_PUB_KEY, nullptr, 0, &pub_key_size);
	std::vector<uint8_t> pub_key_buffer(pub_key_size);
	if (!EVP_PKEY_get_octet_string_param(ec_key, OSSL_PKEY_PARAM_PUB_KEY, pub_key_buffer.data(), pub_key_buffer.size(),
	                                     &pub_key_size))
	{
		size_t group_name_size = 0;
		EVP_PKEY_get_utf8_string_param(ec_key, OSSL_PKEY_PARAM_GROUP_NAME, nullptr, 0, &group_name_size);
		std::vector<char> group_name(group_name_size + 1);
		if (!EVP_PKEY_get_utf8_string_param(ec_key, OSSL_PKEY_PARAM_GROUP_NAME, group_name.data(), group_name.size(),
		                                    &group_name_size))
		{
			return {};
		}

		int group_nid = OBJ_sn2nid(group_name.data());
		if (group_nid == NID_undef)
		{
			return {};
		}

		auto ec_group = EC_GROUP_new_by_curve_name(group_nid);
		if (ec_group == nullptr)
		{
			return {};
		}

		auto pub_key = EC_POINT_new(ec_group);
		if (pub_key == nullptr)
		{
			EC_GROUP_free(ec_group);

			return {};
		}
		BIGNUM* priv_key = nullptr;
		if (!EVP_PKEY_get_bn_param(ec_key, OSSL_PKEY_PARAM_PRIV_KEY, &priv_key))
		{
			EC_POINT_free(pub_key);
			EC_GROUP_free(ec_group);

			return {};
		}

		if (!EC_POINT_mul(ec_group, pub_key, priv_key, nullptr, nullptr, nullptr))
		{
			EC_POINT_free(pub_key);
			EC_GROUP_free(ec_group);

			return {};
		}

		pub_key_size = EC_POINT_point2oct(ec_group, pub_key, POINT_CONVERSION_COMPRESSED, nullptr, 0, nullptr);
		if (pub_key_size == 0)
		{
			EC_POINT_free(pub_key);
			EC_GROUP_free(ec_group);

			return {};
		}
		pub_key_buffer.resize(pub_key_size);
		if (!EC_POINT_point2oct(ec_group, pub_key, POINT_CONVERSION_COMPRESSED, pub_key_buffer.data(),
		                        pub_key_buffer.size(), nullptr))
		{
			EC_POINT_free(pub_key);
			EC_GROUP_free(ec_group);

			return {};
		}

		EC_POINT_free(pub_key);
		EC_GROUP_free(ec_group);

		if (!EVP_PKEY_set_octet_string_param(ec_key, OSSL_PKEY_PARAM_ENCODED_PUBLIC_KEY, pub_key_buffer.data(),
		                                     pub_key_buffer.size()))
		{
			return {};
		}

		auto p_ctx = EVP_PKEY_CTX_new_from_pkey(nullptr, ec_key, nullptr);
		if (p_ctx == nullptr)
		{
			return {};
		}

		if (!EVP_PKEY_public_check_quick(p_ctx))
		{
			EVP_PKEY_CTX_free(p_ctx);

			return {};
		}

		EVP_PKEY_CTX_free(p_ctx);
	}

	return pub_key_buffer;
}
