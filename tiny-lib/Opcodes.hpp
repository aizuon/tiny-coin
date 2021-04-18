#pragma once
#include <cstdint>
#include <type_traits>

enum class Opcode : uint8_t
{
	AddPeerMsg,
	BlockInfoMsg,
	GetActiveChainMsg,
	GetBlockMsg,
	GetMempoolMsg,
	GetUTXOsMsg,
	InvMsg,
	TxInfoMsg
};

typedef std::underlying_type<Opcode>::type OpcodeType;