#include "pch.hpp"
#include "Connection.hpp"

Connection::Connection(boost::asio::io_context& io_context)
	: Socket(io_context)
{
}
