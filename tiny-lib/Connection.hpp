#pragma once
#include <mutex>
#include <boost/asio.hpp>

class Connection
{
public:
	Connection(boost::asio::io_service& io_service);

	boost::asio::ip::tcp::socket Socket;
	boost::asio::streambuf ReadBuffer;

	std::mutex WriteLock;
};
