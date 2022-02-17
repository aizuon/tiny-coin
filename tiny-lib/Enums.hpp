#pragma once
#include <cstdint>
#include <type_traits>

enum class TxStatus : uint8_t
{
	Mempool,
	Mined,
	NotFound
};

enum class NodeType : uint8_t
{
	Unspecified = 0,
	Miner = 1,
	Wallet = 2,
	Full = Miner | Wallet
};

using NodeTypeType = std::underlying_type_t<NodeType>;

inline NodeType operator|(NodeType a, NodeType b)
{
	return static_cast<NodeType>(static_cast<NodeTypeType>(a) | static_cast<NodeTypeType>(b));
}

inline NodeType& operator|=(NodeType& a, NodeType b)
{
	return reinterpret_cast<NodeType&>(reinterpret_cast<int&>(a) |= static_cast<int>(b));
}

inline bool operator&(NodeType a, NodeType b)
{
	return static_cast<NodeTypeType>(a) & static_cast<NodeTypeType>(b);
}
