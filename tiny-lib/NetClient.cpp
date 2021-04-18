#include "pch.hpp"

#include <boost/bind/bind.hpp>
using namespace boost::placeholders;

#include "NetClient.hpp"
#include "IMsg.hpp"
#include "AddPeerMsg.hpp"
#include "GetActiveChainMsg.hpp"
#include "GetBlockMsg.hpp"
#include "GetMempoolMsg.hpp"
#include "GetUTXOsMsg.hpp"
#include "InvMsg.hpp"
#include "Log.hpp"

NetClient::Connection::Connection(boost::asio::io_service& io_service)
	: Socket(io_service), ReadBuffer()
{

}

void NetClient::Run()
{
	IO_Service.run();
}

void NetClient::Connect(const std::string& address, uint16_t port)
{
	auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(address), port);
	auto con = Connections.emplace(Connections.begin(), IO_Service);
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

void NetClient::SendMsgAsync(ConnectionHandle con_handle, const IMsg& msg)
{
	auto serializedMsg = msg.Serialize().GetBuffer();
	auto opcode = static_cast<OpcodeType>(msg.GetOpcode());

	auto msgBuffer = std::make_shared<BinaryBuffer>();
	msgBuffer->Reserve(sizeof(serializedMsg.size()) + sizeof(opcode) + serializedMsg.size());
	msgBuffer->Write(serializedMsg.size());
	msgBuffer->Write(opcode);
	msgBuffer->Write(serializedMsg);

	DoAsyncWrite(con_handle, msgBuffer);
}

boost::asio::io_service NetClient::IO_Service;
boost::asio::ip::tcp::acceptor NetClient::Acceptor = boost::asio::ip::tcp::acceptor(NetClient::IO_Service);
std::list<NetClient::Connection> NetClient::Connections;

void NetClient::HandleConnect(ConnectionHandle con_handle, const boost::system::error_code& err)
{
	if (!err)
	{
		LOG_INFO("Connected to {}", con_handle->Socket.remote_endpoint().address().to_string());
	}
	else
	{
		LOG_ERROR(err.message());;
		Connections.erase(con_handle);
	}
}

void NetClient::StartAccept()
{
	auto con = Connections.emplace(Connections.begin(), IO_Service);
	auto handler = boost::bind(&NetClient::HandleAccept, con, boost::asio::placeholders::error);
	Acceptor.async_accept(con->Socket, handler);
}

void NetClient::HandleAccept(ConnectionHandle con_handle, const boost::system::error_code& err)
{
	if (!err)
	{
		LOG_INFO("Incoming connection from {}", con_handle->Socket.remote_endpoint().address().to_string());
	}
	else
	{
		LOG_ERROR(err.message());;
		Connections.erase(con_handle);
	}
	StartAccept();
}

void NetClient::DoAsyncRead(ConnectionHandle con_handle)
{
	auto handler = boost::bind(&NetClient::HandleRead, con_handle, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
	boost::asio::async_read(con_handle->Socket, con_handle->ReadBuffer, handler);
}

void NetClient::HandleRead(ConnectionHandle con_handle, const boost::system::error_code& err, size_t bytes_transfered)
{
	if (bytes_transfered > 0)
	{
		auto& readBuffer = con_handle->ReadBuffer;

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

						HandleMsg(con_handle, buffer);
					}
				}
			}
		}
	}

	if (!err)
	{
		DoAsyncRead(con_handle);
	}
	else
	{
		LOG_ERROR(err.message());
		Connections.erase(con_handle);
	}
}

void NetClient::HandleMsg(ConnectionHandle con_handle, BinaryBuffer& msg_buffer)
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
	msg->Handle(con_handle);
}

void NetClient::DoAsyncWrite(ConnectionHandle con_handle, const std::shared_ptr<BinaryBuffer> msg_buffer)
{
	auto handler = boost::bind(&NetClient::HandleWrite, con_handle, msg_buffer, boost::asio::placeholders::error);
	boost::asio::async_write(con_handle->Socket, boost::asio::buffer(msg_buffer->GetBuffer()), handler);
	DoAsyncRead(con_handle);
}

void NetClient::HandleWrite(ConnectionHandle con_handle, const std::shared_ptr<BinaryBuffer> msg_buffer, const boost::system::error_code& err)
{
	if (!err)
	{
		if (con_handle->Socket.is_open())
		{
		}
	}
	else
	{
		LOG_ERROR(err.message());;
		Connections.erase(con_handle);
	}
}
