#pragma once
#include <cstdint>
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

	static std::vector<std::shared_ptr<Connection>> Connections;

	static void Run();

	static void Connect(const std::string& address, uint16_t port);

	static void Listen(uint16_t port);

	static std::shared_ptr<Connection> GetRandomConnection();

	static void SendMsgAsync(const std::shared_ptr<Connection>& con, const IMsg& msg);
	static void SendMsgRandomAsync(const IMsg& msg);

private:
	static boost::asio::io_service IO_Service;
	static boost::asio::ip::tcp::acceptor Acceptor;

	static void HandleConnect(const std::shared_ptr<Connection>& con, const boost::system::error_code& err);

	static void StartAccept();
	static void HandleAccept(const std::shared_ptr<Connection>& con, const boost::system::error_code& err);

	static void DoAsyncRead(const std::shared_ptr<Connection>& con);
	static void HandleRead(const std::shared_ptr<Connection>& con, const boost::system::error_code& err, size_t bytes_transfered);

	static void HandleMsg(const std::shared_ptr<Connection>& con, BinaryBuffer& msg_buffer);

	static void DoAsyncWrite(const std::shared_ptr<Connection>& con, const std::shared_ptr<BinaryBuffer>& msg_buffer);
	static void HandleWrite(const std::shared_ptr<Connection>& con, const std::shared_ptr<BinaryBuffer> msg_buffer, const boost::system::error_code& err);

	static void RemoveConnection(const std::shared_ptr<Connection>& con);
};
