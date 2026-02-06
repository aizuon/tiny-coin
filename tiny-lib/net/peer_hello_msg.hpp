#pragma once
#include "net/i_msg.hpp"
#include "core/enums.hpp"

class PeerHelloMsg : public IMsg
{
public:
	NodeType node_type;

	~PeerHelloMsg() override = default;

	void handle(const std::shared_ptr<Connection>& con) override;
	BinaryBuffer serialize() const override;
	bool deserialize(BinaryBuffer& buffer) override;

	Opcode get_opcode() const override;
};
