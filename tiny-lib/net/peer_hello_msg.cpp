#include "net/peer_hello_msg.hpp"

#include <algorithm>

#include "net/net_client.hpp"
#include "wallet/node_config.hpp"

PeerHelloMsg::PeerHelloMsg()
	: node_type(NodeConfig::type)
{}

void PeerHelloMsg::handle(const std::shared_ptr<Connection>& con)
{
	con->node_type = node_type;
	if (con->node_type & NodeType::Miner)
	{
		std::scoped_lock lock(NetClient::connections_mutex);

		const auto vec_it = std::find(NetClient::miner_connections.begin(),
			NetClient::miner_connections.end(), con);

		if (vec_it == NetClient::miner_connections.end())
		{
			NetClient::miner_connections.push_back(con);
		}
	}
}

BinaryBuffer PeerHelloMsg::serialize() const
{
	BinaryBuffer buffer;

	buffer.write(static_cast<NodeTypeType>(node_type));

	return buffer;
}

bool PeerHelloMsg::deserialize(BinaryBuffer& buffer)
{
	NodeTypeType raw_node_type = 0;
	if (!buffer.read(raw_node_type))
		return false;

	node_type = static_cast<::NodeType>(raw_node_type);

	return true;
}

Opcode PeerHelloMsg::get_opcode() const
{
	return Opcode::PeerHelloMsg;
}
