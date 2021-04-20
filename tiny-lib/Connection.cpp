#include "pch.hpp"

#include "Connection.hpp"

Connection::Connection(boost::asio::io_service& io_service)
	: Socket(io_service)
{

}