#pragma once
#include <cstdint>
#include <type_traits>

enum class TxStatus : uint8_t
{
	Mempool,
	Mined,
	NotFound
};

enum NodeType : uint8_t
{
	Unspecified = 0,
	Miner = 1,
	Wallet = 2,
	Full = Miner | Wallet //TODO: implement full node
};

using NodeTypeType = std::underlying_type<NodeType>::type;

inline NodeType operator|(NodeType a, NodeType b)
{
	return static_cast<NodeType>(static_cast<NodeTypeType>(a) | static_cast<NodeTypeType>(b));
}
