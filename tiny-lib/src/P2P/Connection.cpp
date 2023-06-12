#include "pch.hpp"
#include "P2P/Connection.hpp"

Connection::Connection(boost::asio::io_service& io_service)
	: Socket(io_service)
{
}
