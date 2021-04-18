#pragma once
#include <cstdint>
#include <type_traits>

enum class Opcode : uint8_t
{
	AddPeerMsg,
	GetActiveChainMsg,
	GetBlockMsg,
	GetMempoolMsg,
	GetUTXOsMsg,
	InvMsg
};

typedef std::underlying_type<Opcode>::type OpcodeType;