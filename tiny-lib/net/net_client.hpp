#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include "net/connection.hpp"
#include "net/i_msg.hpp"

class BinaryBuffer;

class NetClient
{
public:
	static const std::vector<std::pair<std::string, uint16_t>> initial_peers;

	static void run_async();
	static void stop();

	static void connect(const std::string& address, uint16_t port);
	static void listen_async(uint16_t port);

	static void send_msg(const std::shared_ptr<Connection>& con, const IMsg& msg);
	static bool send_msg_random(const IMsg& msg);

	static void broadcast_msg(const IMsg& msg);

	static std::recursive_mutex connections_mutex;

	static std::vector<std::shared_ptr<Connection>> miner_connections;

private:
	static std::string magic;

	static boost::asio::io_context io_context;
	static boost::thread io_thread;
	static boost::asio::ip::tcp::acceptor acceptor;

	static std::vector<std::shared_ptr<Connection>> connections;

	static std::shared_ptr<Connection> get_random_connection();

	static void start_accept();
	static void handle_accept(std::shared_ptr<Connection> con, const boost::system::error_code& err);

	static void do_async_read(std::shared_ptr<Connection> con);
	static void handle_read(std::shared_ptr<Connection> con, const boost::system::error_code& err,
		size_t bytes_transferred);

	static void handle_msg(const std::shared_ptr<Connection>& con, BinaryBuffer& msg_buffer);

	static BinaryBuffer prepare_send_buffer(const IMsg& msg);
	static void write(const std::shared_ptr<Connection>& con, const BinaryBuffer& msg_buffer);

	static void remove_connection(const std::shared_ptr<Connection>& con);
};
