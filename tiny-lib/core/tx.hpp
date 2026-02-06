#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include "util/i_deserializable.hpp"
#include "util/i_serializable.hpp"
#include "core/tx_in.hpp"
#include "core/tx_out.hpp"

class UnspentTxOut;

class Tx : public ISerializable, public IDeserializable
{
public:
	Tx() = default;
	Tx(const std::vector<std::shared_ptr<TxIn>>& tx_ins, const std::vector<std::shared_ptr<TxOut>>& tx_outs,
		int64_t lock_time);

	Tx(const Tx& other);
	Tx& operator=(const Tx& other);
	Tx(Tx&& other) noexcept;
	Tx& operator=(Tx&& other) noexcept;

	std::vector<std::shared_ptr<TxIn>> tx_ins;
	std::vector<std::shared_ptr<TxOut>> tx_outs;

	int64_t lock_time = 0;

	bool is_coinbase() const;
	bool signals_rbf() const;

	std::string id() const;

	void validate_basics(bool coinbase = false) const;

	struct ValidateRequest
	{
		bool as_coinbase = false;
		bool allow_utxo_from_mempool = true;
		std::vector<std::shared_ptr<Tx>> siblings_in_block;
	};

	void validate(const ValidateRequest& req) const;

	bool is_final() const;
	void check_lock_time(int64_t block_height, int64_t block_mtp) const;

	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	static std::shared_ptr<Tx> create_coinbase(const std::string& pay_to_addr, uint64_t value, int64_t height);

	bool operator==(const Tx& obj) const;

private:
	void validate_signature_for_spend(const std::shared_ptr<TxIn>& tx_in, const std::shared_ptr<UnspentTxOut>& utxo) const;

	mutable std::string cached_id_;
	mutable std::mutex cached_id_mutex_;

	auto tied() const
	{
		return std::tie(lock_time);
	}
};
