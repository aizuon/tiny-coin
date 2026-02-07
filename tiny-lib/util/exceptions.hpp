#pragma once
#include <stdexcept>
#include <memory>

class Tx;
class Block;

class TxUnlockException : public std::runtime_error
{
public:
	TxUnlockException(const char* msg);
};

class TxValidationException : public std::runtime_error
{
public:
	TxValidationException(const char* msg);
	TxValidationException(const char* msg, const std::shared_ptr<Tx>& to_orphan);

	std::shared_ptr<Tx> to_orphan;
};

class BlockValidationException : public std::runtime_error
{
public:
	BlockValidationException(const char* msg);
	BlockValidationException(const char* msg, const std::shared_ptr<Block>& to_orphan);

	std::shared_ptr<Block> to_orphan;
};
