#include "core/tx.hpp"

#include <ranges>
#include <unordered_set>
#include <fmt/format.h>

#include "core/chain.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/sig_cache.hpp"
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

Tx::Tx(const Tx& other)
	: tx_ins(other.tx_ins), tx_outs(other.tx_outs), lock_time(other.lock_time)
{}

Tx& Tx::operator=(const Tx& other)
{
	if (this != &other)
	{
		tx_ins = other.tx_ins;
		tx_outs = other.tx_outs;
		lock_time = other.lock_time;
		cached_id_.clear();
	}
	return *this;
}

Tx::Tx(Tx&& other) noexcept
	: tx_ins(std::move(other.tx_ins)), tx_outs(std::move(other.tx_outs)), lock_time(other.lock_time)
{}

Tx& Tx::operator=(Tx&& other) noexcept
{
	if (this != &other)
	{
		tx_ins = std::move(other.tx_ins);
		tx_outs = std::move(other.tx_outs);
		lock_time = other.lock_time;
		cached_id_.clear();
	}
	return *this;
}

bool Tx::is_coinbase() const
{
	return tx_ins.size() == 1 && tx_ins.front()->to_spend == nullptr;
}

bool Tx::signals_rbf() const
{
	for (const auto& tx_in : tx_ins)
	{
		if (tx_in->sequence != TxIn::SEQUENCE_FINAL)
			return true;
	}
	return false;
}

std::string Tx::id() const
{
	std::scoped_lock lock(cached_id_mutex_);

	if (cached_id_.empty())
		cached_id_ = Utils::byte_array_to_hex_string(SHA256::double_hash_binary(serialize().get_buffer()));

	return cached_id_;
}

void Tx::validate_basics(bool coinbase /*= false*/) const
{
	if (tx_outs.empty() || (tx_ins.empty() && !coinbase))
		throw TxValidationException("Missing tx_outs or tx_ins");

	if (serialize().get_size() > NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES)
		throw TxValidationException("Too large");

	if (!coinbase && tx_ins.size() > 1)
	{
		std::unordered_set<std::string> seen_outpoints;
		seen_outpoints.reserve(tx_ins.size());
		for (const auto& tx_in : tx_ins)
		{
			if (tx_in->to_spend == nullptr)
				continue;
			const auto key = tx_in->to_spend->tx_id + ":" + std::to_string(tx_in->to_spend->tx_out_idx);
			if (!seen_outpoints.insert(key).second)
				throw TxValidationException("Duplicate input");
		}
	}

	uint64_t total_spent = 0;
	for (const auto& tx_out : tx_outs)
	{
		if (tx_out->value > NetParams::MAX_MONEY)
			throw TxValidationException("Single output value too high");
		if (total_spent + tx_out->value < total_spent)
			throw TxValidationException("Output total overflow");
		total_spent += tx_out->value;
	}

	if (total_spent > NetParams::MAX_MONEY)
		throw TxValidationException("Spent value too high");
}

bool Tx::is_final() const
{
	if (lock_time == 0)
		return true;

	for (const auto& tx_in : tx_ins)
	{
		if (tx_in->sequence != TxIn::SEQUENCE_FINAL)
			return false;
	}
	return true;
}

void Tx::check_lock_time(int64_t block_height, int64_t block_mtp) const
{
	if (is_final())
		return;

	if (lock_time < NetParams::LOCKTIME_THRESHOLD)
	{
		if (lock_time > block_height)
			throw TxValidationException(
				fmt::format("Transaction lock time {} not reached (current height {})", lock_time, block_height).c_str());
	}
	else
	{
		if (lock_time > block_mtp)
			throw TxValidationException(
				fmt::format("Transaction lock time {} not reached (median time past {})", lock_time, block_mtp).c_str());
	}
}

void Tx::check_sequence_locks(int64_t block_height, int64_t block_mtp) const
{
	if (is_coinbase())
		return;

	for (uint32_t i = 0; i < tx_ins.size(); i++)
	{
		const auto& tx_in = tx_ins[i];

		if (!tx_in->has_relative_locktime())
			continue;

		auto utxo = UTXO::find_in_map(tx_in->to_spend);
		if (utxo == nullptr)
			continue;

		const int64_t utxo_height = utxo->height;

		if (tx_in->is_time_based_locktime())
		{
			const uint32_t utxo_block_idx = static_cast<uint32_t>(utxo_height) - 1;
			const int64_t start_mtp = (utxo_block_idx > 0)
				? Chain::get_median_time_past_at_height(utxo_block_idx - 1)
				: 0;

			const int64_t required_time =
				start_mtp + (static_cast<int64_t>(tx_in->relative_locktime_value())
					<< TxIn::SEQUENCE_LOCKTIME_GRANULARITY);

			if (required_time > block_mtp)
				throw TxValidationException(
					fmt::format("TxIn {} relative time-lock not satisfied "
						"(required MTP {}, current MTP {})", i, required_time, block_mtp).c_str());
		}
		else
		{
			const int64_t required_height =
				utxo_height + static_cast<int64_t>(tx_in->relative_locktime_value());

			if (required_height > block_height)
				throw TxValidationException(
					fmt::format("TxIn {} relative block-lock not satisfied "
						"(required height {}, current height {})", i, required_height, block_height).c_str());
		}
	}
}

void Tx::validate(const ValidateRequest& req) const
{
	validate_basics(req.as_coinbase);

	if (!req.as_coinbase)
	{
		check_lock_time(Chain::get_current_height(), Chain::get_median_time_past(11));
		check_sequence_locks(Chain::get_current_height(), Chain::get_median_time_past(11));
	}

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

			if (utxo == nullptr && req.allow_utxo_from_mempool)
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

		if (!req.skip_sig_validation)
		{
			try
			{
				validate_signature_for_spend(tx_in, utxo);
			}
			catch (const TxUnlockException& ex)
			{
				LOG_ERROR(ex.what());

				throw TxValidationException(fmt::format("TxIn {} not a valid spend of UTXO", i).c_str());
			}
		}

		if (available_to_spend + utxo->tx_out->value < available_to_spend)
			throw TxValidationException("Input total overflow");
		available_to_spend += utxo->tx_out->value;
	}

	uint64_t total_spent = 0;
	for (const auto& tx_out : tx_outs)
	{
		if (total_spent + tx_out->value < total_spent)
			throw TxValidationException("Output total overflow");
		total_spent += tx_out->value;
	}

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
	uint32_t tx_ins_size = 0;
	if (!buffer.read_size(tx_ins_size))
		return false;

	std::vector<std::shared_ptr<TxIn>> new_tx_ins;
	new_tx_ins.reserve(tx_ins_size);
	for (uint32_t i = 0; i < tx_ins_size; i++)
	{
		auto tx_in = std::make_shared<TxIn>();
		if (!tx_in->deserialize(buffer))
			return false;
		new_tx_ins.push_back(std::move(tx_in));
	}

	uint32_t tx_outs_size = 0;
	if (!buffer.read_size(tx_outs_size))
		return false;

	std::vector<std::shared_ptr<TxOut>> new_tx_outs;
	new_tx_outs.reserve(tx_outs_size);
	for (uint32_t i = 0; i < tx_outs_size; i++)
	{
		auto tx_out = std::make_shared<TxOut>();
		if (!tx_out->deserialize(buffer))
			return false;
		new_tx_outs.push_back(std::move(tx_out));
	}

	int64_t new_lock_time = 0;
	if (!buffer.read(new_lock_time))
		return false;

	tx_ins = std::move(new_tx_ins);
	tx_outs = std::move(new_tx_outs);
	lock_time = new_lock_time;
	cached_id_.clear();

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

	if (SigCache::contains(tx_in->unlock_sig, spend_msg, tx_in->unlock_pub_key))
		return;

	if (!ECDSA::verify_sig(tx_in->unlock_sig, spend_msg, tx_in->unlock_pub_key))
	{
		LOG_ERROR("Key verification failed");

		throw TxUnlockException("Signature does not match");
	}

	SigCache::add(tx_in->unlock_sig, spend_msg, tx_in->unlock_pub_key);
}
