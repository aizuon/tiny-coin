#include "core/tx.hpp"

#include <ranges>
#include <fmt/format.h>

#include "core/chain.hpp"
#include "crypto/ecdsa.hpp"
#include "util/exceptions.hpp"
#include "util/log.hpp"
#include "core/mempool.hpp"
#include "net/msg_serializer.hpp"
#include "core/net_params.hpp"
#include "crypto/sha256.hpp"
#include "core/unspent_tx_out.hpp"
#include "util/utils.hpp"
#include "wallet/wallet.hpp"

Tx::Tx(const std::vector<std::shared_ptr<TxIn>>& tx_ins, const std::vector<std::shared_ptr<TxOut>>& tx_outs,
	int64_t lock_time)
	: tx_ins(tx_ins), tx_outs(tx_outs), lock_time(lock_time)
{}

bool Tx::is_coinbase() const
{
	return tx_ins.size() == 1 && tx_ins.front()->to_spend == nullptr;
}

std::string Tx::id() const
{
	return Utils::byte_array_to_hex_string(SHA256::double_hash_binary(serialize().get_buffer()));
}

void Tx::validate_basics(bool coinbase /*= false*/) const
{
	if (tx_outs.empty() || (tx_ins.empty() && !coinbase))
		throw TxValidationException("Missing tx_outs or tx_ins");

	if (serialize().get_size() > NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES)
		throw TxValidationException("Too large");

	uint64_t total_spent = 0;
	for (const auto& tx_out : tx_outs)
		total_spent += tx_out->value;

	if (total_spent > NetParams::MAX_MONEY)
		throw TxValidationException("Spent value too high");
}

void Tx::validate(const ValidateRequest& req) const
{
	validate_basics(req.as_coinbase);

	uint64_t available_to_spend = 0;
	for (uint32_t i = 0; i < tx_ins.size(); i++)
	{
		const auto& tx_in = tx_ins[i];

		auto utxo = UTXO::find_in_map(tx_in->to_spend);
		if (utxo == nullptr)
		{
			if (!req.siblings_in_block.empty())
			{
				utxo = UTXO::find_in_list(tx_in, req.siblings_in_block);
			}

			if (req.allow_utxo_from_mempool)
			{
				utxo = Mempool::find_utxo_in_mempool(tx_in->to_spend);
			}

			if (utxo == nullptr)
				throw TxValidationException(
					fmt::format("Unable to find any UTXO for TxIn {}, orphaning transaction", i).c_str(),
					std::make_shared<Tx>(*this));
		}

		if (utxo->is_coinbase && Chain::get_current_height() - utxo->height < NetParams::COINBASE_MATURITY)
			throw TxValidationException("Coinbase UTXO not ready for spending");

		try
		{
			validate_signature_for_spend(tx_in, utxo);
		}
		catch (const TxUnlockException& ex)
		{
			LOG_ERROR(ex.what());

			throw TxValidationException(fmt::format("TxIn {} not a valid spend of UTXO", i).c_str());
		}

		available_to_spend += utxo->tx_out->value;
	}

	uint64_t total_spent = 0;
	for (const auto& tx_out : tx_outs)
		total_spent += tx_out->value;

	if (available_to_spend < total_spent)
		throw TxValidationException("Spent value more than available");
}

BinaryBuffer Tx::serialize() const
{
	BinaryBuffer buffer;

	buffer.write_size(static_cast<uint32_t>(tx_ins.size()));
	for (const auto& tx_in : tx_ins)
		buffer.write_raw(tx_in->serialize().get_buffer());

	buffer.write_size(static_cast<uint32_t>(tx_outs.size()));
	for (const auto& tx_out : tx_outs)
		buffer.write_raw(tx_out->serialize().get_buffer());

	buffer.write(lock_time);

	return buffer;
}

bool Tx::deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	uint32_t tx_ins_size = 0;
	if (!buffer.read_size(tx_ins_size))
	{
		*this = std::move(copy);

		return false;
	}
	tx_ins.clear();
	tx_ins.reserve(tx_ins_size);
	for (uint32_t i = 0; i < tx_ins_size; i++)
	{
		auto tx_in = std::make_shared<TxIn>();
		if (!tx_in->deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		tx_ins.push_back(tx_in);
	}

	uint32_t tx_outs_size = 0;
	if (!buffer.read_size(tx_outs_size))
	{
		*this = std::move(copy);

		return false;
	}
	tx_outs.clear();
	tx_outs.reserve(tx_outs_size);
	for (uint32_t i = 0; i < tx_outs_size; i++)
	{
		auto tx_out = std::make_shared<TxOut>();
		if (!tx_out->deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		tx_outs.push_back(tx_out);
	}

	if (!buffer.read(lock_time))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

std::shared_ptr<Tx> Tx::create_coinbase(const std::string& pay_to_addr, uint64_t value, int64_t height)
{
	BinaryBuffer tx_in_unlock_sig;
	tx_in_unlock_sig.write(height);
	const auto tx_in = std::make_shared<TxIn>(nullptr, tx_in_unlock_sig.get_buffer(), std::vector<uint8_t>(), -1);

	const auto tx_out = std::make_shared<TxOut>(value, pay_to_addr);

	std::vector tx_ins{ tx_in };
	std::vector tx_outs{ tx_out };
	auto tx = std::make_shared<Tx>(tx_ins, tx_outs, 0);

	return tx;
}

bool Tx::operator==(const Tx& obj) const
{
	if (this == &obj)
	{
		return true;
	}

	if (tied() != obj.tied())
		return false;

	if (tx_ins.size() != obj.tx_ins.size())
		return false;
	for (uint32_t i = 0; i < tx_ins.size(); i++)
	{
		if (*tx_ins[i] != *obj.tx_ins[i])
		{
			return false;
		}
	}

	if (tx_outs.size() != obj.tx_outs.size())
		return false;
	for (uint32_t i = 0; i < tx_outs.size(); i++)
	{
		if (*tx_outs[i] != *obj.tx_outs[i])
		{
			return false;
		}
	}

	return true;
}

void Tx::validate_signature_for_spend(const std::shared_ptr<TxIn>& tx_in, const std::shared_ptr<UTXO>& utxo) const
{
	const auto pub_key_as_addr = Wallet::pub_key_to_address(tx_in->unlock_pub_key);
	if (pub_key_as_addr != utxo->tx_out->to_address)
		throw TxUnlockException("Public key does not match");

	const auto spend_msg = MsgSerializer::build_spend_msg(tx_in->to_spend, tx_in->unlock_pub_key, tx_in->sequence, tx_outs);
	if (!ECDSA::verify_sig(tx_in->unlock_sig, spend_msg, tx_in->unlock_pub_key))
	{
		LOG_ERROR("Key verification failed");

		throw TxUnlockException("Signature does not match");
	}
}
