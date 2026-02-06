#include "util/exceptions.hpp"

#include "core/block.hpp"
#include "core/tx.hpp"

TxUnlockException::TxUnlockException(const char* msg)
	: std::exception(msg)
{
}

TxValidationException::TxValidationException(const char* msg)
	: std::exception(msg), to_orphan(nullptr)
{
}

TxValidationException::TxValidationException(const char* msg, const std::shared_ptr<Tx>& to_orphan)
	: std::exception(msg), to_orphan(to_orphan)
{
}

BlockValidationException::BlockValidationException(const char* msg)
	: std::exception(msg), to_orphan(nullptr)
{
}

BlockValidationException::BlockValidationException(const char* msg, const std::shared_ptr<Block>& to_orphan)
	: std::exception(msg), to_orphan(to_orphan)
{
}
