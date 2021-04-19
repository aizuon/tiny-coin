#include "pch.hpp"

#include <boost/bind/bind.hpp>
using namespace boost::placeholders;

#include "NetClient.hpp"
#include "IMsg.hpp"
#include "AddPeerMsg.hpp"
#include "BlockInfoMsg.hpp"
#include "GetActiveChainMsg.hpp"
#include "GetBlockMsg.hpp"
#include "GetMempoolMsg.hpp"
#include "GetUTXOsMsg.hpp"
#include "InvMsg.hpp"
#include "SendActiveChainMsg.hpp"
#include "SendMempoolMsg.hpp"
#include "SendUTXOsMsg.hpp"
#include "TxInfoMsg.hpp"
#include "Log.hpp"
#include "Random.hpp"

NetClient::Connection::Connection(boost::asio::io_service& io_service)
	: Socket(io_service), ReadBuffer()
{

}

std::vector<std::shared_ptr<NetClient::Connection>> NetClient::Connections;

void NetClient::Run()
{
	IO_Service.run();
}

void NetClient::Connect(const std::string& address, uint16_t port)
{
	auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(address), port);
	auto con = Connections.emplace_back(std::make_shared<Connection>(IO_Service));
	auto handler = boost::bind(&NetClient::HandleConnect, con, boost::asio::placeholders::error);
	con->Socket.async_connect(endpoint, handler);
}

void NetClient::Listen(uint16_t port)
{
	auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
	Acceptor.open(endpoint.protocol());
	Acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	Acceptor.bind(endpoint);
	Acceptor.listen();
	StartAccept();
}

void NetClient::SendMsgAsync(const std::shared_ptr<Connection>& con, const IMsg& msg)
{
	auto serializedMsg = msg.Serialize().GetBuffer();
	auto opcode = static_cast<OpcodeType>(msg.GetOpcode());

	auto msgBuffer = std::make_shared<BinaryBuffer>();
	msgBuffer->Reserve(sizeof(serializedMsg.size()) + sizeof(opcode) + serializedMsg.size());
	msgBuffer->Write(serializedMsg.size());
	msgBuffer->Write(opcode);
	msgBuffer->Write(serializedMsg);

	DoAsyncWrite(con, msgBuffer);
}

void NetClient::SendMsgRandomAsync(const IMsg& msg)
{
	auto con = GetRandomConnection();
	SendMsgAsync(con, msg);
}

std::shared_ptr<NetClient::Connection> NetClient::GetRandomConnection()
{
	auto randomIdx = Random::GetInt(0, Connections.size() - 1);
	return Connections[randomIdx];
}

boost::asio::io_service NetClient::IO_Service;
boost::asio::ip::tcp::acceptor NetClient::Acceptor = boost::asio::ip::tcp::acceptor(NetClient::IO_Service);

void NetClient::HandleConnect(const std::shared_ptr<Connection>& con, const boost::system::error_code& err)
{
	if (!err)
	{
		LOG_INFO("Connected to {}", con->Socket.remote_endpoint().address().to_string());
	}
	else
	{
		LOG_ERROR(err.message());;
		RemoveConnection(con);
	}
}

void NetClient::StartAccept()
{
	auto con = Connections.emplace_back(std::make_shared<Connection>(IO_Service));
	auto handler = boost::bind(&NetClient::HandleAccept, con, boost::asio::placeholders::error);
	Acceptor.async_accept(con->Socket, handler);
}

void NetClient::HandleAccept(const std::shared_ptr<Connection>& con, const boost::system::error_code& err)
{
	if (!err)
	{
		LOG_INFO("Incoming connection from {}", con->Socket.remote_endpoint().address().to_string());
	}
	else
	{
		LOG_ERROR(err.message());;
		RemoveConnection(con);
	}
	StartAccept();
}

void NetClient::DoAsyncRead(const std::shared_ptr<Connection>& con)
{
	auto handler = boost::bind(&NetClient::HandleRead, con, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
	boost::asio::async_read(con->Socket, con->ReadBuffer, handler);
}

void NetClient::HandleRead(const std::shared_ptr<Connection>& con, const boost::system::error_code& err, size_t bytes_transfered)
{
	if (bytes_transfered > 0)
	{
		auto& readBuffer = con->ReadBuffer;

		size_t msgSize = 0;
		if (readBuffer.size() > sizeof(msgSize))
		{
			BinaryBuffer sizeBuffer;
			sizeBuffer.GrowTo(sizeof(msgSize));
			if (boost::asio::buffer_copy(boost::asio::buffer(sizeBuffer.GetWritableBuffer()), readBuffer.data()) > sizeof(msgSize))
			{
				if (sizeBuffer.Read(msgSize))
				{
					if (readBuffer.size() >= sizeof(msgSize) + msgSize)
					{
						readBuffer.consume(sizeof(msgSize));

						BinaryBuffer buffer;
						buffer.GrowTo(msgSize);
						boost::asio::buffer_copy(boost::asio::buffer(buffer.GetWritableBuffer()), readBuffer.data(), msgSize);
						readBuffer.consume(msgSize);

						HandleMsg(con, buffer);
					}
				}
			}
		}
	}

	if (!err)
	{
		DoAsyncRead(con);
	}
	else
	{
		LOG_ERROR(err.message());
		RemoveConnection(con);
	}
}

void NetClient::HandleMsg(const std::shared_ptr<Connection>& con, BinaryBuffer& msg_buffer)
{
	OpcodeType opcode = 0;
	if (!msg_buffer.Read(opcode))
	{
		LOG_ERROR("No opcode");

		return;
	}
	Opcode opcode2 = static_cast<Opcode>(opcode);

	std::unique_ptr<IMsg> msg = nullptr;
	switch (opcode2)
	{
	case Opcode::AddPeerMsg:
	{
		msg = std::make_unique<AddPeerMsg>();

		break;
	}
	case Opcode::BlockInfoMsg:
	{
		msg = std::make_unique<BlockInfoMsg>();

		break;
	}
	case Opcode::GetActiveChainMsg:
	{
		msg = std::make_unique<GetActiveChainMsg>();

		break;
	}
	case Opcode::GetBlockMsg:
	{
		msg = std::make_unique<GetBlockMsg>();

		break;
	}
	case Opcode::GetMempoolMsg:
	{
		msg = std::make_unique<GetMempoolMsg>();

		break;
	}
	case Opcode::GetUTXOsMsg:
	{
		msg = std::make_unique<GetUTXOsMsg>();

		break;
	}
	case Opcode::InvMsg:
	{
		msg = std::make_unique<InvMsg>();

		break;
	}
	case Opcode::SendActiveChainMsg:
	{
		msg = std::make_unique<SendActiveChainMsg>();

		break;
	}
	case Opcode::SendMempoolMsg:
	{
		msg = std::make_unique<SendMempoolMsg>();

		break;
	}
	case Opcode::SendUTXOsMsg:
	{
		msg = std::make_unique<SendUTXOsMsg>();

		break;
	}
	case Opcode::TxInfoMsg:
	{
		msg = std::make_unique<TxInfoMsg>();

		break;
	}
	default:
	{
		LOG_ERROR("Unknown opcode {}", opcode2);

		return;
	}
	}

	if (!msg->Deserialize(msg_buffer))
	{
		LOG_ERROR("Unable to deserialize opcode {}", opcode2);

		return;
	}
	msg->Handle(con);
}

void NetClient::DoAsyncWrite(const std::shared_ptr<Connection>& con, const std::shared_ptr<BinaryBuffer>& msg_buffer)
{
	auto handler = boost::bind(&NetClient::HandleWrite, con, msg_buffer, boost::asio::placeholders::error);
	boost::asio::async_write(con->Socket, boost::asio::buffer(msg_buffer->GetBuffer()), handler);
	DoAsyncRead(con);
}

void NetClient::HandleWrite(const std::shared_ptr<Connection>& con, const std::shared_ptr<BinaryBuffer> msg_buffer, const boost::system::error_code& err)
{
	if (!err)
	{
		if (con->Socket.is_open())
		{
		}
	}
	else
	{
		LOG_ERROR(err.message());
		RemoveConnection(con);
	}
}

void NetClient::RemoveConnection(const std::shared_ptr<Connection>& con)
{
	auto list_it = std::find_if(Connections.begin(), Connections.end(),
		[&con](const std::shared_ptr<Connection>& o)
		{
			return con->Socket.remote_endpoint() == o->Socket.remote_endpoint();
		});

	if (list_it != Connections.end())
		Connections.erase(list_it);
}
