#pragma once
#include <exception>
#include <memory>

class Tx;
class Block;

class TxUnlockException : public std::exception
{
public:
	TxUnlockException(const char* msg);
};

class TxValidationException : public std::exception
{
public:
	TxValidationException(const char* msg);
	TxValidationException(const char* msg, const std::shared_ptr<Tx>& toOrphan);

	std::shared_ptr<Tx> ToOrphan;
};

class BlockValidationException : public std::exception
{
public:
	BlockValidationException(const char* msg);
	BlockValidationException(const char* msg, const std::shared_ptr<Block>& toOrphan);

	std::shared_ptr<Block> ToOrphan;
};
