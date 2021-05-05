#pragma once
#include <memory>
#include <exception>

class Tx;
class Block;

class TxUnlockException : public std::exception
{
public:
	TxUnlockException(const char* msg);

private:
};

class TxValidationException : public std::exception
{
public:
	TxValidationException(const char* msg);
	TxValidationException(const char* msg, std::shared_ptr<Tx> toOrphan);

	std::shared_ptr<Tx> ToOrphan;

private:
};

class BlockValidationException : public std::exception
{
public:
	BlockValidationException(const char* msg);
	BlockValidationException(const char* msg, std::shared_ptr<Block> toOrphan);

	std::shared_ptr<Block> ToOrphan;

private:
};
