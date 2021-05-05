#include "pch.hpp"
#include "NetClient.hpp"

#include <mutex>
#include <boost/bind/bind.hpp>

#include "AddPeerMsg.hpp"
#include "BinaryBuffer.hpp"
#include "BlockInfoMsg.hpp"
#include "GetActiveChainMsg.hpp"
#include "GetBlockMsg.hpp"
#include "GetMempoolMsg.hpp"
#include "GetUTXOsMsg.hpp"
#include "IMsg.hpp"
#include "InvMsg.hpp"
#include "Log.hpp"
#include "Random.hpp"
#include "SendActiveChainMsg.hpp"
#include "SendMempoolMsg.hpp"
#include "SendUTXOsMsg.hpp"
#include "TxInfoMsg.hpp"

using namespace boost::placeholders;

const std::vector<std::pair<std::string, uint16_t>> NetClient::InitialPeers = std::vector<std::pair<
	std::string, uint16_t>>{
	{"127.0.0.1", 9900}, {"127.0.0.1", 9901}, {"127.0.0.1", 9902}, {"127.0.0.1", 9903}, {"127.0.0.1", 9904}
};

std::string NetClient::Magic = "\r\n\r\n";

std::vector<std::shared_ptr<Connection>> NetClient::Connections;

boost::asio::io_service NetClient::IO_Service;
boost::thread NetClient::IO_Thread;
boost::asio::ip::tcp::acceptor NetClient::Acceptor = boost::asio::ip::tcp::acceptor(IO_Service);

void NetClient::RunAsync()
{
	IO_Thread = boost::thread(boost::bind(&boost::asio::io_service::run, &IO_Service));
}

void NetClient::Stop()
{
	for (const auto& con : Connections)
	{
		if (con->Socket.is_open())
		{
			con->Socket.shutdown(boost::asio::socket_base::shutdown_both);
		}
	}

	IO_Service.stop();
	IO_Thread.join();

	Connections.clear();
}

void NetClient::Connect(const std::string& address, uint16_t port)
{
	const auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(address), port);
	auto con = std::make_shared<Connection>(IO_Service);
	try
	{
		con->Socket.connect(endpoint);
	}
	catch (const boost::system::system_error&)
	{
		return;
	}
	con->Socket.set_option(boost::asio::ip::tcp::no_delay(true));
	Connections.push_back(con);
	DoAsyncRead(con);
}

void NetClient::ListenAsync(uint16_t port)
{
	const auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
	Acceptor.open(endpoint.protocol());
	Acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	Acceptor.bind(endpoint);
	Acceptor.listen();
	StartAccept();
}

void NetClient::SendMsg(std::shared_ptr<Connection>& con, const IMsg& msg)
{
	const auto msgBuffer = PrepareSendBuffer(msg);

	Write(con, msgBuffer);
}

bool NetClient::SendMsgRandom(const IMsg& msg)
{
	auto con = GetRandomConnection();
	if (con == nullptr)
		return false;
	

	SendMsg(con, msg);

	return true;
}

void NetClient::BroadcastMsg(const IMsg& msg)
{
	if (Connections.empty())
		return;

	const auto msgBuffer = PrepareSendBuffer(msg);

	for (auto& con : Connections)
		Write(con, msgBuffer);
}

std::shared_ptr<Connection> NetClient::GetRandomConnection()
{
	if (Connections.empty())
		return nullptr;

	const auto randomIdx = Random::GetInt(0, Connections.size() - 1);
	return Connections[randomIdx];
}

void NetClient::StartAccept()
{
	auto con = std::make_shared<Connection>(IO_Service);
	auto handler = boost::bind(&NetClient::HandleAccept, con, boost::asio::placeholders::error);
	Acceptor.async_accept(con->Socket, handler);
}

void NetClient::HandleAccept(std::shared_ptr<Connection>& con, const boost::system::error_code& err)
{
	if (!err)
	{
		auto& soc = con->Socket;

		LOG_INFO("Incoming connection from {}:{}", soc.remote_endpoint().address().to_string(),
		         soc.remote_endpoint().port());

		soc.set_option(boost::asio::ip::tcp::no_delay(true));
		Connections.push_back(con);
		DoAsyncRead(con);
	}
	else
	{
		LOG_ERROR(err.message());
	}
	StartAccept();
}

void NetClient::DoAsyncRead(std::shared_ptr<Connection>& con)
{
	auto handler = boost::bind(&NetClient::HandleRead, con, boost::asio::placeholders::error,
	                           boost::asio::placeholders::bytes_transferred);
	boost::asio::async_read_until(con->Socket, con->ReadBuffer, Magic, handler);
}

void NetClient::HandleRead(std::shared_ptr<Connection>& con, const boost::system::error_code& err,
                           size_t bytes_transferred)
{
	if (!err)
	{
		auto& readBuffer = con->ReadBuffer;
		if (bytes_transferred > Magic.size() && readBuffer.size() >= bytes_transferred)
		{
			BinaryBuffer buffer;
			buffer.GrowTo(bytes_transferred - (Magic.size()));
			boost::asio::buffer_copy(boost::asio::buffer(buffer.GetWritableBuffer()), readBuffer.data());

			HandleMsg(con, buffer);

			readBuffer.consume(bytes_transferred);
		}

		DoAsyncRead(con);
	}
	else
	{
		if (err != boost::asio::error::eof && err != boost::asio::error::shut_down && err !=
			boost::asio::error::connection_reset && err != boost::asio::error::operation_aborted)
			LOG_ERROR(err.message());

		RemoveConnection(con);
	}
}

void NetClient::HandleMsg(std::shared_ptr<Connection>& con, BinaryBuffer& msg_buffer)
{
	OpcodeType opcode = 0;
	if (!msg_buffer.Read(opcode))
	{
		LOG_ERROR("No opcode");

		return;
	}
	auto opcode2 = static_cast<Opcode>(opcode);

	std::unique_ptr<IMsg> msg;
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

BinaryBuffer NetClient::PrepareSendBuffer(const IMsg& msg)
{
	const auto serializedMsg = msg.Serialize().GetBuffer();
	auto opcode = static_cast<OpcodeType>(msg.GetOpcode());

	BinaryBuffer msgBuffer;
	msgBuffer.Reserve(sizeof(opcode) + serializedMsg.size() + Magic.size());
	msgBuffer.Write(opcode);
	msgBuffer.WriteRaw(serializedMsg);
	msgBuffer.WriteRaw(Magic);

	return msgBuffer;
}

void NetClient::Write(std::shared_ptr<Connection>& con, const BinaryBuffer& msg_buffer)
{
	std::lock_guard lock(con->WriteLock);

	boost::system::error_code err;
	boost::asio::write(con->Socket, boost::asio::buffer(msg_buffer.GetBuffer()), err);
	if (err)
	{
		if (err != boost::asio::error::shut_down && err != boost::asio::error::connection_reset && err !=
			boost::asio::error::operation_aborted)
			LOG_ERROR(err.message());

		RemoveConnection(con);
	}
}

void NetClient::RemoveConnection(std::shared_ptr<Connection>& con)
{
	const auto vec_it = std::ranges::find_if(Connections,
	                                         [&con](const std::shared_ptr<Connection>& o)
	                                         {
		                                         return con.get() == o.get();
	                                         });

	if (vec_it != Connections.end())
	{
		auto& soc = con->Socket;

		LOG_INFO("Peer {}:{} disconnected", soc.remote_endpoint().address().to_string(), soc.remote_endpoint().port());

		if (soc.is_open())
		{
			soc.close();
		}
		Connections.erase(vec_it);
	}
}
