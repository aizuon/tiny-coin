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

TxValidationException::TxValidationException(const char* msg, const std::shared_ptr<Tx>& to_orphan)
	: std::exception(msg), ToOrphan(to_orphan)
{
}

BlockValidationException::BlockValidationException(const char* msg)
	: std::exception(msg), ToOrphan(nullptr)
{
}

BlockValidationException::BlockValidationException(const char* msg, const std::shared_ptr<Block>& to_orphan)
	: std::exception(msg), ToOrphan(to_orphan)
{
}
