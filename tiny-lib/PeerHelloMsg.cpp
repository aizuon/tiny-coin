#include "pch.hpp"
#include "PeerHelloMsg.hpp"

#include <ranges>

#include "NetClient.hpp"
#include "NodeConfig.hpp"

void PeerHelloMsg::Handle(std::shared_ptr<Connection>& con)
{
	con->NodeType = NodeType;
	if (con->NodeType & NodeType::Miner)
	{
		std::scoped_lock lock(NetClient::ConnectionsMutex);

		const auto vec_it = std::ranges::find_if(NetClient::MinerConnections,
		                                         [&con](const std::shared_ptr<Connection>& o)
		                                         {
			                                         return con == o;
		                                         });

		if (vec_it == NetClient::MinerConnections.end())
		{
			NetClient::MinerConnections.push_back(con);
		}
	}
}

BinaryBuffer PeerHelloMsg::Serialize() const
{
	BinaryBuffer buffer;

	buffer.Write(static_cast<NodeTypeType>(NodeConfig::Type));

	return buffer;
}

bool PeerHelloMsg::Deserialize(BinaryBuffer& buffer)
{
	auto copy = *this;

	NodeTypeType node_type = 0;
	if (!buffer.Read(node_type))
	{
		*this = std::move(copy);

		return false;
	}
	NodeType = static_cast<::NodeType>(node_type);

	return true;
}

Opcode PeerHelloMsg::GetOpcode() const
{
	return Opcode::PeerHelloMsg;
}
