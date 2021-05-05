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
	SendActiveChainMsg,
	SendMempoolMsg,
	SendUTXOsMsg,
	TxInfoMsg
};

using OpcodeType = std::underlying_type<Opcode>::type;
