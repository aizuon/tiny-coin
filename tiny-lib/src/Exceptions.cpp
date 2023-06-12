#include "pch.hpp"
#include "Exceptions.hpp"

#include "BlockChain/Block.hpp"
#include "Tx/Tx.hpp"

TxUnlockException::TxUnlockException(const char* msg)
	: std::runtime_error(msg)
{
}

TxValidationException::TxValidationException(const char* msg)
	: std::runtime_error(msg), ToOrphan(nullptr)
{
}

TxValidationException::TxValidationException(const char* msg, const std::shared_ptr<Tx>& to_orphan)
	: std::runtime_error(msg), ToOrphan(to_orphan)
{
}

BlockValidationException::BlockValidationException(const char* msg)
	: std::runtime_error(msg), ToOrphan(nullptr)
{
}

BlockValidationException::BlockValidationException(const char* msg, const std::shared_ptr<Block>& to_orphan)
	: std::runtime_error(msg), ToOrphan(to_orphan)
{
}
