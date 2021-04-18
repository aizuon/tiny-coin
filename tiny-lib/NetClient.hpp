#pragma once
#include <cstdint>
#include <list>
#include <memory>
#include <boost/asio.hpp>

#include "BinaryBuffer.hpp"

class IMsg;

class NetClient
{
public:
	class Connection
	{
	public:
		Connection(boost::asio::io_service& io_service);

		boost::asio::ip::tcp::socket Socket;
		boost::asio::streambuf ReadBuffer;
	};

	using ConnectionHandle = std::list<Connection>::iterator;

	static void Run();

	static void Connect(const std::string& address, uint16_t port);

	static void Listen(uint16_t port);

	static void SendMsgAsync(ConnectionHandle con_handle, const IMsg& msg);

private:
	static boost::asio::io_service IO_Service;
	static boost::asio::ip::tcp::acceptor Acceptor;
	static std::list<Connection> Connections;

	static void HandleConnect(ConnectionHandle con_handle, const boost::system::error_code& err);

	static void StartAccept();
	static void HandleAccept(ConnectionHandle con_handle, const boost::system::error_code& err);

	static void DoAsyncRead(ConnectionHandle con_handle);
	static void HandleRead(ConnectionHandle con_handle, const boost::system::error_code& err, size_t bytes_transfered);

	static void HandleMsg(ConnectionHandle con_handle, BinaryBuffer& msg_buffer);

	static void DoAsyncWrite(ConnectionHandle con_handle, const std::shared_ptr<BinaryBuffer> msg_buffer);
	static void HandleWrite(ConnectionHandle con_handle, const std::shared_ptr<BinaryBuffer> msg_buffer, const boost::system::error_code& err);
};
