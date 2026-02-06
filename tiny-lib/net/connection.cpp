#include "net/connection.hpp"

Connection::Connection(boost::asio::io_context& io_context)
	: socket(io_context)
{}
