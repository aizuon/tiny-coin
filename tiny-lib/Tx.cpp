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

Tx::Tx(const std::vector<std::shared_ptr<TxIn>>& tx_ins, const std::vector<std::shared_ptr<TxOut>>& tx_outs,
       int64_t lock_time)
	: TxIns(tx_ins), TxOuts(tx_outs), LockTime(lock_time)
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

	uint64_t total_spent = 0;
	for (const auto& tx_out : TxOuts)
		total_spent += tx_out->Value;

	if (total_spent > NetParams::MAX_MONEY)
		throw TxValidationException("Spent value too high");
}

void Tx::Validate(const ValidateRequest& req) const
{
	ValidateBasics(req.AsCoinbase);

	uint64_t avaliable_to_spend = 0;
	for (uint32_t i = 0; i < TxIns.size(); i++)
	{
		const auto& tx_in = TxIns[i];

		auto utxo = UTXO::FindInMap(tx_in->ToSpend);
		if (utxo == nullptr)
		{
			if (!req.SiblingsInBlock.empty())
			{
				utxo = UTXO::FindInList(tx_in, req.SiblingsInBlock);
			}

			if (req.Allow_UTXO_FromMempool)
			{
				utxo = Mempool::Find_UTXO_InMempool(tx_in->ToSpend);
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
			ValidateSignatureForSpend(tx_in, utxo);
		}
		catch (const TxUnlockException& ex)
		{
			LOG_ERROR(ex.what());

			throw TxValidationException(fmt::format("TxIn not a valid spend of UTXO").c_str());
		}

		avaliable_to_spend += utxo->TxOut->Value;
	}

	uint64_t total_spent = 0;
	for (const auto& tx_out : TxOuts)
		total_spent += tx_out->Value;

	if (avaliable_to_spend < total_spent)
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

	uint32_t tx_ins_size = 0;
	if (!buffer.ReadSize(tx_ins_size))
	{
		*this = std::move(copy);

		return false;
	}
	if (!TxIns.empty())
		TxIns.clear();
	TxIns.reserve(tx_ins_size);
	for (uint32_t i = 0; i < tx_ins_size; i++)
	{
		auto tx_in = std::make_shared<TxIn>();
		if (!tx_in->Deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		TxIns.push_back(tx_in);
	}

	uint32_t tx_outs_size = 0;
	if (!buffer.ReadSize(tx_outs_size))
	{
		*this = std::move(copy);

		return false;
	}
	if (!TxOuts.empty())
		TxOuts.clear();
	TxOuts.reserve(tx_outs_size);
	for (uint32_t i = 0; i < tx_outs_size; i++)
	{
		auto tx_out = std::make_shared<TxOut>();
		if (!tx_out->Deserialize(buffer))
		{
			*this = std::move(copy);

			return false;
		}
		TxOuts.push_back(tx_out);
	}

	if (!buffer.Read(LockTime))
	{
		*this = std::move(copy);

		return false;
	}

	return true;
}

std::shared_ptr<Tx> Tx::CreateCoinbase(const std::string& pay_to_addr, uint64_t value, int64_t height)
{
	BinaryBuffer tx_in_unlock_sig;
	tx_in_unlock_sig.Reserve(sizeof(height));
	tx_in_unlock_sig.Write(height);
	const auto tx_in = std::make_shared<TxIn>(nullptr, tx_in_unlock_sig.GetBuffer(), std::vector<uint8_t>(), -1);

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

void Tx::ValidateSignatureForSpend(std::shared_ptr<TxIn> tx_in, std::shared_ptr<UTXO> utxo) const
{
	const auto pub_key_as_addr = Wallet::PubKeyToAddress(tx_in->UnlockPubKey);
	if (pub_key_as_addr != utxo->TxOut->ToAddress)
		throw TxUnlockException("Public key does not match");

	const auto spend_msg = MsgSerializer::BuildSpendMsg(tx_in->ToSpend, tx_in->UnlockPubKey, tx_in->Sequence, TxOuts);
	if (!ECDSA::VerifySig(tx_in->UnlockSig, spend_msg, tx_in->UnlockPubKey))
	{
		LOG_ERROR("Key verification failed");

		throw TxUnlockException("Signature does not match");
	}
}
