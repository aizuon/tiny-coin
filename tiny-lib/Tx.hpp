#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#include "ISerializable.hpp"
#include "TxIn.hpp"
#include "TxOut.hpp"

class Tx : public ISerializable
{
public:
	Tx(const std::vector<std::shared_ptr<TxIn>>& txIns, const std::vector<std::shared_ptr<TxOut>>& txOuts, int64_t lockTime);

	std::vector<std::shared_ptr<TxIn>> TxIns;
	std::vector<std::shared_ptr<TxOut>> TxOuts;

	int64_t LockTime;

	bool IsCoinbase() const;

	std::string Id() const;

	void Validate(bool coinbase = false) const;

	std::vector<uint8_t> Serialize() const override;

	static std::shared_ptr<Tx> CreateCoinbase(const std::string& PayToAddr, uint64_t value, int32_t height);
};