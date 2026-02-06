#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "core/block.hpp"
#include "util/i_deserializable.hpp"
#include "util/i_serializable.hpp"
#include "core/tx.hpp"
#include "core/tx_in.hpp"
#include "core/tx_out.hpp"
#include "core/tx_out_point.hpp"

class UnspentTxOut : public ISerializable, public IDeserializable
{
public:
	UnspentTxOut() = default;
	UnspentTxOut(std::shared_ptr<TxOut> tx_out, std::shared_ptr<TxOutPoint> tx_out_point, bool is_coinbase,
		int64_t height);

	std::shared_ptr<TxOut> tx_out;

	std::shared_ptr<TxOutPoint> tx_out_point;

	bool is_coinbase = false;

	int64_t height = -1;

	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	static std::unordered_map<std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>, TxOutPointHash, TxOutPointEqual> map;

	static std::recursive_mutex mutex;

	static void add_to_map(const std::shared_ptr<::TxOut>& tx_out, const std::string& tx_id, int64_t idx, bool is_coinbase,
		int64_t height);
	static void remove_from_map(const std::string& tx_id, int64_t idx);

	static std::shared_ptr<UnspentTxOut> find_in_list(const std::shared_ptr<TxIn>& tx_in,
		const std::vector<std::shared_ptr<Tx>>& txs);
	static std::shared_ptr<UnspentTxOut> find_in_map(const std::shared_ptr<::TxOutPoint>& to_spend);

	static std::shared_ptr<::TxOut> find_tx_out_in_block(const std::shared_ptr<Block>& block,
		const std::shared_ptr<TxIn>& tx_in);
	static std::shared_ptr<::TxOut> find_tx_out_in_map(const std::shared_ptr<TxIn>& tx_in);
	static std::shared_ptr<::TxOut> find_tx_out_in_map_or_block(const std::shared_ptr<Block>& block,
		const std::shared_ptr<TxIn>& tx_in);

	bool operator==(const UnspentTxOut& obj) const;

private:
	auto tied() const
	{
		return std::tie(is_coinbase, height);
	}
};

using UTXO = UnspentTxOut;
