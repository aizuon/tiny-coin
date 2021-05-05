#include "pch.hpp"
#include "Exceptions.hpp"

#include "Block.hpp"
#include "Tx.hpp"

TxUnlockException::TxUnlockException(const char* msg)
	: std::exception(msg)
{
}

TxValidationException::TxValidationException(const char* msg)
	: std::exception(msg), ToOrphan(nullptr)
{
}

TxValidationException::TxValidationException(const char* msg, std::shared_ptr<Tx> toOrphan)
	: std::exception(msg), ToOrphan(toOrphan)
{
}

BlockValidationException::BlockValidationException(const char* msg)
	: std::exception(msg), ToOrphan(nullptr)
{
}

BlockValidationException::BlockValidationException(const char* msg, std::shared_ptr<Block> toOrphan)
	: std::exception(msg), ToOrphan(toOrphan)
{
}
