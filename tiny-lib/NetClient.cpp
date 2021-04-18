#include "pch.hpp"

#include <boost/bind/bind.hpp>
using namespace boost::placeholders;

#include "NetClient.hpp"

NetClient::Connection::Connection(boost::asio::io_service& io_service)
	: Socket(io_service), ReadBuffer()
{

}

void NetClient::Connect(const std::string& address, uint16_t port)
{
	auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(address), port);
	auto con = Connections.emplace(Connections.begin(), IO_Service);
	auto handler = boost::bind(&NetClient::HandleConnect, con, boost::asio::placeholders::error);
	con->Socket.async_connect(endpoint, handler);
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

void NetClient::Listen(uint16_t port)
{
	auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
	Acceptor.open(endpoint.protocol());
	Acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	Acceptor.bind(endpoint);
	Acceptor.listen();
	StartAccept();
}

void NetClient::Run()
{
	IO_Service.run();
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
		std::vector<uint8_t> buffer(con_handle->ReadBuffer.size());
		boost::asio::buffer_copy(boost::asio::buffer(buffer), con_handle->ReadBuffer.data());
		//TODO: handle packet
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

void NetClient::DoAsyncWrite(ConnectionHandle con_handle, const std::shared_ptr<std::vector<uint8_t>> msg_buffer)
{
	auto handler = boost::bind(&NetClient::HandleWrite, con_handle, msg_buffer, boost::asio::placeholders::error);
	boost::asio::async_write(con_handle->Socket, boost::asio::buffer(*msg_buffer), handler);
	DoAsyncRead(con_handle);
}

void NetClient::HandleWrite(ConnectionHandle con_handle, const std::shared_ptr<std::vector<uint8_t>> msg_buffer, const boost::system::error_code& err)
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
