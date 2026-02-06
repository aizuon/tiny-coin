#pragma once
#include <mutex>
#include <boost/asio.hpp>

#include "core/enums.hpp"

class Connection
{
public:
	Connection(boost::asio::io_context& io_context);

	boost::asio::ip::tcp::socket socket;
	boost::asio::streambuf read_buffer;

	std::mutex write_mutex;

	NodeType node_type = NodeType::Unspecified;
};
