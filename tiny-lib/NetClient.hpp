#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

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

	static const std::vector<std::pair<std::string, uint16_t>> InitialPeers;

	static void RunAsync();
	static void Stop();

	static void Connect(const std::string& address, uint16_t port);
	static void ListenAsync(uint16_t port);

	static void SendMsgAsync(std::shared_ptr<Connection>& con, const IMsg& msg);
	static bool SendMsgRandomAsync(const IMsg& msg);

	static void BroadcastMsgAsync(const IMsg& msg);

private:
	static std::string Magic;

	static boost::asio::io_service IO_Service;
	static boost::thread IO_Thread;
	static boost::asio::ip::tcp::acceptor Acceptor;

	static std::vector<std::shared_ptr<Connection>> Connections;

	static std::shared_ptr<Connection> GetRandomConnection();

	static void StartAccept();
	static void HandleAccept(std::shared_ptr<Connection>& con, const boost::system::error_code& err);

	static void DoAsyncRead(std::shared_ptr<Connection>& con);
	static void HandleRead(std::shared_ptr<Connection>& con, const boost::system::error_code& err, size_t bytes_transferred);

	static void HandleMsg(std::shared_ptr<Connection>& con, BinaryBuffer& msg_buffer);

	static void DoAsyncWrite(std::shared_ptr<Connection>& con, const std::shared_ptr<BinaryBuffer>& msg_buffer);
	static void HandleWrite(std::shared_ptr<Connection>& con, const std::shared_ptr<BinaryBuffer> msg_buffer, const boost::system::error_code& err, size_t bytes_transferred);

	static void RemoveConnection(std::shared_ptr<Connection>& con);
};
