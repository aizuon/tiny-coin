#pragma once
#include <mutex>
#include <boost/asio.hpp>

#include "Enums.hpp"

class Connection
{
public:
	Connection(boost::asio::io_context& io_context);

	boost::asio::ip::tcp::socket Socket;
	boost::asio::streambuf ReadBuffer;

	std::mutex WriteMutex;

	NodeType NodeType = NodeType::Unspecified;
};
