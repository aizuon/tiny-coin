#include <cstdint>
#include <list>
#include <memory>
#include <boost/asio.hpp>

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

	using ConnectionHandle = std::list<Connection>::iterator;

	static void Run();

	static void Connect(const std::string& address, uint16_t port);
	static void HandleConnect(ConnectionHandle con_handle, const boost::system::error_code& err);

	static void Listen(uint16_t port);

	static void StartAccept();
	static void HandleAccept(ConnectionHandle con_handle, const boost::system::error_code& err);

	static void DoAsyncRead(ConnectionHandle con_handle);
	static void HandleRead(ConnectionHandle con_handle, const boost::system::error_code& err, size_t bytes_transfered);

	static void DoAsyncWrite(ConnectionHandle con_handle, const std::shared_ptr<std::vector<uint8_t>> msg_buffer);
	static void HandleWrite(ConnectionHandle con_handle, const std::shared_ptr<std::vector<uint8_t>> msg_buffer, const boost::system::error_code& err);

private:
	static boost::asio::io_service IO_Service;
	static boost::asio::ip::tcp::acceptor Acceptor;
	static std::list<Connection> Connections;
};
