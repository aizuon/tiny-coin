#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include "Connection.hpp"
#include "IMsg.hpp"

#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")

class BinaryBuffer;

class NetClient
{
public:
	static const std::vector<std::pair<std::string, uint16_t>> InitialPeers;

	static void RunAsync();
	static void Stop();

	static void Connect(const std::string& address, uint16_t port);
	static void ListenAsync(uint16_t port);

	static void SendMsg(std::shared_ptr<Connection>& con, const IMsg& msg);
	static bool SendMsgRandom(const IMsg& msg);

	static void BroadcastMsg(const IMsg& msg);

	static std::recursive_mutex ConnectionsMutex;

	static std::vector<std::shared_ptr<Connection>> MinerConnections;

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
	static void HandleRead(std::shared_ptr<Connection>& con, const boost::system::error_code& err,
	                       size_t bytes_transferred);

	static void HandleMsg(std::shared_ptr<Connection>& con, BinaryBuffer& msg_buffer);

	static BinaryBuffer PrepareSendBuffer(const IMsg& msg);
	static void Write(std::shared_ptr<Connection>& con, const BinaryBuffer& msg_buffer);

	static void RemoveConnection(std::shared_ptr<Connection>& con);
};
