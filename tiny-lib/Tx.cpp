#include "pch.hpp"
#include "Tx.hpp"

#include <ranges>
#include <fmt/format.h>

#include "Chain.hpp"
#include "ECDSA.hpp"
#include "Exceptions.hpp"
#include "Log.hpp"
#include "Mempool.hpp"
#include "MsgSerializer.hpp"
#include "NetParams.hpp"
#include "SHA256.hpp"
#include "UnspentTxOut.hpp"
#include "Utils.hpp"
#include "Wallet.hpp"

Tx::Tx(const std::vector<std::shared_ptr<TxIn>>& txIns, const std::vector<std::shared_ptr<TxOut>>& txOuts,
       int64_t lockTime)
	: TxIns(txIns), TxOuts(txOuts), LockTime(lockTime)
{
}

bool Tx::IsCoinbase() const
{
	return TxIns.size() == 1 && TxIns.front()->ToSpend == nullptr;
}

std::string Tx::Id() const
{
	return Utils::ByteArrayToHexString(SHA256::DoubleHashBinary(Serialize().GetBuffer()));
}

void Tx::ValidateBasics(bool coinbase /*= false*/) const
{
	if (TxOuts.empty() || (TxIns.empty() && !coinbase))
		throw TxValidationException("Missing TxOuts or TxIns");

	if (Serialize().GetSize() > NetParams::MAX_BLOCK_SERIALIZED_SIZE_IN_BYTES)
		throw TxValidationException("Too large");

	uint64_t totalSpent = 0;
	for (const auto& tx_out : TxOuts)
		totalSpent += tx_out->Value;

	if (totalSpent > NetParams::MAX_MONEY)
		throw TxValidationException("Spent value too high");
}

void Tx::Validate(const ValidateRequest& req) const
{
	ValidateBasics(req.AsCoinbase);

	uint64_t avaliableToSpend = 0;
	for (uint32_t i = 0; i < TxIns.size(); i++)
	{
		const auto& txIn = TxIns[i];

		auto utxo = UTXO::FindInMap(txIn->ToSpend);
		if (utxo == nullptr)
		{
			if (!req.SiblingsInBlock.empty())
			{
				utxo = UTXO::FindInList(txIn, req.SiblingsInBlock);
			}

			if (req.Allow_UTXO_FromMempool)
			{
				utxo = Mempool::Find_UTXO_InMempool(txIn->ToSpend);
			}

			if (utxo == nullptr)
				throw TxValidationException(
					fmt::format("Unable to find any UTXO for TxIn {}, orphaning transaction", i).c_str(),
					std::make_shared<Tx>(*this));
		}

		if (utxo->IsCoinbase && Chain::GetCurrentHeight() - utxo->Height < NetParams::COINBASE_MATURITY)
			throw TxValidationException("Coinbase UTXO not ready for spending");

		try
		{
			ValidateSignatureForSpend(txIn, utxo);
		}
		catch (const TxUnlockException&)
		{
			throw TxValidationException(fmt::format("TxIn not a valid spend of UTXO").c_str());
		}

		avaliableToSpend += utxo->TxOut->Value;
	}

	uint64_t totalSpent = 0;
	for (const auto& txOut : TxOuts)
		totalSpent += txOut->Value;

	if (avaliableToSpend < totalSpent)
		throw TxValidationException("Spent value more than available");
}

BinaryBuffer Tx::Serialize() const
{
	BinaryBuffer buffer;

	buffer.WriteSize(TxIns.size());
	for (const auto& tx_in : TxIns)
		buffer.WriteRaw(tx_in->Serialize().GetBuffer());

	buffer.WriteSize(TxOuts.size());
	for (const auto& tx_out : TxOuts)
		buffer.WriteRaw(tx_out->Serialize().GetBuffer());

	buffer.Write(LockTime);

	return buffer;
}

bool Tx::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	uint32_t txInsSize = 0;
	if (!buffer.ReadSize(txInsSize))
	{
		*this = std::move(copy);

		return false;
	}
	if (!TxIns.empty())
		TxIns.clear();
	TxIns.reserve(txInsSize);
	for (uint32_t i = 0; i < txInsSize; i++)
	{
		auto txIn = std::make_shared<TxIn>();
		if (!txIn->Deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		TxIns.push_back(txIn);
	}

	uint32_t txOutsSize = 0;
	if (!buffer.ReadSize(txOutsSize))
	{
		*this = std::move(copy);

		return false;
	}
	if (!TxOuts.empty())
		TxOuts.clear();
	TxOuts.reserve(txOutsSize);
	for (uint32_t i = 0; i < txOutsSize; i++)
	{
		auto txOut = std::make_shared<TxOut>();
		if (!txOut->Deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		TxOuts.push_back(txOut);
	}

	if (!buffer.Read(LockTime))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

std::shared_ptr<Tx> Tx::CreateCoinbase(const std::string& payToAddr, uint64_t value, int64_t height)
{
	BinaryBuffer tx_in_unlockSig;
	tx_in_unlockSig.Reserve(sizeof(height));
	tx_in_unlockSig.Write(height);
	const auto tx_in = std::make_shared<TxIn>(nullptr, tx_in_unlockSig.GetBuffer(), std::vector<uint8_t>(), 0);

	const auto tx_out = std::make_shared<TxOut>(value, payToAddr);

	std::vector tx_ins{tx_in};
	std::vector tx_outs{tx_out};
	auto tx = std::make_shared<Tx>(tx_ins, tx_outs, -1);

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

	if (TxIns.size() != obj.TxIns.size())
		return false;
	for (uint32_t i = 0; i < TxIns.size(); i++)
	{
		if (*TxIns[i] != *obj.TxIns[i])
		{
			return false;
		}
	}

	if (TxOuts.size() != obj.TxOuts.size())
		return false;
	for (uint32_t i = 0; i < TxOuts.size(); i++)
	{
		if (*TxOuts[i] != *obj.TxOuts[i])
		{
			return false;
		}
	}

	return true;
}

void Tx::ValidateSignatureForSpend(const std::shared_ptr<TxIn>& txIn, const std::shared_ptr<UTXO>& utxo) const
{
	const auto pubKeyAsAddr = Wallet::PubKeyToAddress(txIn->UnlockPubKey);
	if (pubKeyAsAddr != utxo->TxOut->ToAddress)
		throw TxUnlockException("Public key does not match");

	const auto spend_msg = MsgSerializer::BuildSpendMsg(txIn->ToSpend, txIn->UnlockPubKey, txIn->Sequence, TxOuts);
	if (!ECDSA::VerifySig(txIn->UnlockSig, spend_msg, txIn->UnlockPubKey))
	{
		LOG_ERROR("Key verification failed");

		throw TxUnlockException("Signature does not match");
	}
}
