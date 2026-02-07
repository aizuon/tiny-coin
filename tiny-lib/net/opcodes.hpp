#pragma once
#include <cstdint>
#include <type_traits>

enum class Opcode : uint8_t
{
	BlockInfoMsg,
	GetActiveChainMsg,
	GetBlockMsg,
	GetMempoolMsg,
	GetUTXOsMsg,
	InvMsg,
	PeerAddMsg,
	PeerHelloMsg,
	SendActiveChainMsg,
	SendMempoolMsg,
	SendUTXOsMsg,
	TxInfoMsg
};

using OpcodeType = std::underlying_type_t<Opcode>;
