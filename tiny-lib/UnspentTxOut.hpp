#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "Block.hpp"
#include "IDeserializable.hpp"
#include "ISerializable.hpp"
#include "Tx.hpp"
#include "TxIn.hpp"
#include "TxOut.hpp"
#include "TxOutPoint.hpp"

class UnspentTxOut : public ISerializable, public IDeserializable
{
public:
	UnspentTxOut() = default;
	UnspentTxOut(std::shared_ptr<TxOut> tx_out, std::shared_ptr<TxOutPoint> tx_out_point, bool is_coinbase,
	             int64_t height);

	std::shared_ptr<TxOut> TxOut;

	std::shared_ptr<TxOutPoint> TxOutPoint;

	bool IsCoinbase = false;

	int64_t Height = -1;

	BinaryBuffer Serialize() const override;
	bool Deserialize(BinaryBuffer& buffer) override;

	static std::unordered_map<std::shared_ptr<::TxOutPoint>, std::shared_ptr<UnspentTxOut>> Map;

	static std::recursive_mutex Mutex;

	static void AddToMap(std::shared_ptr<::TxOut> tx_out, const std::string& tx_id, int64_t idx, bool is_coinbase,
	                     int64_t height);
	static void RemoveFromMap(const std::string& tx_id, int64_t idx);

	static std::shared_ptr<UnspentTxOut> FindInList(std::shared_ptr<TxIn> tx_in,
	                                                const std::vector<std::shared_ptr<Tx>>& txs);
	static std::shared_ptr<UnspentTxOut> FindInMap(std::shared_ptr<::TxOutPoint> to_spend);

	static std::shared_ptr<::TxOut> FindTxOutInBlock(std::shared_ptr<Block> block,
	                                                 std::shared_ptr<TxIn> tx_in);
	static std::shared_ptr<::TxOut> FindTxOutInMap(std::shared_ptr<TxIn> tx_in);
	static std::shared_ptr<::TxOut> FindTxOutInMapOrBlock(std::shared_ptr<Block> block,
	                                                      std::shared_ptr<TxIn> tx_in);

	bool operator==(const UnspentTxOut& obj) const;

private:
	auto tied() const
	{
		return std::tie(IsCoinbase, Height);
	}
};

using UTXO = UnspentTxOut;
