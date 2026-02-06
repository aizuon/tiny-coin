#include "net/net_client.hpp"

#include <algorithm>
#include <cstring>
#include <boost/bind/bind.hpp>

#include "util/binary_buffer.hpp"
#include "net/block_info_msg.hpp"
#include "net/get_active_chain_msg.hpp"
#include "net/get_block_msg.hpp"
#include "net/get_mempool_msg.hpp"
#include "net/get_utxos_msg.hpp"
#include "net/i_msg.hpp"
#include "net/inv_msg.hpp"
#include "util/log.hpp"
#include "net/peer_add_msg.hpp"
#include "net/peer_hello_msg.hpp"
#include "util/random.hpp"
#include "net/send_active_chain_msg.hpp"
#include "net/send_mempool_msg.hpp"
#include "net/send_utxos_msg.hpp"
#include "crypto/sha256.hpp"
#include "net/tx_info_msg.hpp"

using namespace boost::placeholders;

const std::vector<std::pair<std::string, uint16_t>> NetClient::initial_peers = std::vector<std::pair<
	std::string, uint16_t>>{
		{ "127.0.0.1", 9900 }, { "127.0.0.1", 9901 }, { "127.0.0.1", 9902 }, { "127.0.0.1", 9903 }, { "127.0.0.1", 9904 }
};

std::recursive_mutex NetClient::connections_mutex;

std::vector<std::shared_ptr<Connection>> NetClient::connections;
std::vector<std::shared_ptr<Connection>> NetClient::miner_connections;

boost::asio::io_context NetClient::io_context;
boost::thread NetClient::io_thread;
boost::asio::ip::tcp::acceptor NetClient::acceptor = boost::asio::ip::tcp::acceptor(io_context);

void NetClient::run_async()
{
	io_thread = boost::thread(boost::bind(&boost::asio::io_context::run, &io_context));
}

void NetClient::stop()
{
	{
		std::scoped_lock lock(connections_mutex);

		for (const auto& con : connections)
		{
			auto& soc = con->socket;

			if (soc.is_open())
			{
				boost::system::error_code ec;
				soc.shutdown(boost::asio::socket_base::shutdown_both, ec);
				soc.close(ec);
			}
		}
	}

	io_context.stop();
	if (io_thread.joinable())
		io_thread.join();

	{
		std::scoped_lock lock(connections_mutex);

		miner_connections.clear();
		connections.clear();
	}
}

void NetClient::connect(const std::string& address, uint16_t port)
{
	const auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address_v4(address), port);
	auto con = std::make_shared<Connection>(io_context);
	try
	{
		con->socket.connect(endpoint);
	}
	catch (const boost::system::system_error&)
	{
		return;
	}
	con->socket.set_option(boost::asio::ip::tcp::no_delay(true));
	{
		std::scoped_lock lock(connections_mutex);

		connections.push_back(con);
	}
	send_msg(con, PeerHelloMsg());
	do_async_read_header(con);
}

void NetClient::listen_async(uint16_t port)
{
	const auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
	acceptor.open(endpoint.protocol());
	acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor.bind(endpoint);
	acceptor.listen();
	start_accept();
}

void NetClient::send_msg(const std::shared_ptr<Connection>& con, const IMsg& msg)
{
	const auto msg_buffer = prepare_send_buffer(msg);

	write(con, msg_buffer);
}

bool NetClient::send_msg_random(const IMsg& msg)
{
	auto con = get_random_connection();
	if (con == nullptr)
		return false;

	send_msg(con, msg);

	return true;
}

void NetClient::broadcast_msg(const IMsg& msg)
{
	const auto msg_buffer = prepare_send_buffer(msg);

	std::scoped_lock lock(connections_mutex);

	for (auto& con : miner_connections)
	{
		write(con, msg_buffer);
	}
}

std::shared_ptr<Connection> NetClient::get_random_connection()
{
	std::scoped_lock lock(connections_mutex);

	if (miner_connections.empty())
		return nullptr;

	const auto random_idx = Random::get_int(0, miner_connections.size() - 1);
	return miner_connections[random_idx];
}

void NetClient::start_accept()
{
	const auto con = std::make_shared<Connection>(io_context);
	auto handler = boost::bind(&NetClient::handle_accept, con, boost::asio::placeholders::error);
	acceptor.async_accept(con->socket, handler);
}

void NetClient::handle_accept(std::shared_ptr<Connection> con, const boost::system::error_code& err)
{
	if (!err)
	{
		auto& soc = con->socket;

		LOG_TRACE("Incoming connection from {}:{}", soc.remote_endpoint().address().to_string(),
			soc.remote_endpoint().port());

		soc.set_option(boost::asio::ip::tcp::no_delay(true));
		{
			std::scoped_lock lock(connections_mutex);

			connections.push_back(con);
		}
		send_msg(con, PeerHelloMsg());
		do_async_read_header(con);
	}
	else
	{
		LOG_ERROR(err.message());
	}
	start_accept();
}

void NetClient::do_async_read_header(std::shared_ptr<Connection> con)
{
	auto handler = boost::bind(&NetClient::handle_read_header, con, boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred);
	boost::asio::async_read(con->socket, con->read_buffer,
		boost::asio::transfer_exactly(HEADER_SIZE), handler);
}

void NetClient::handle_read_header(std::shared_ptr<Connection> con, const boost::system::error_code& err,
	size_t bytes_transferred)
{
	if (!err)
	{
		if (bytes_transferred < HEADER_SIZE || con->read_buffer.size() < HEADER_SIZE)
		{
			remove_connection(con);
			return;
		}

		std::array<uint8_t, HEADER_SIZE> header{};
		boost::asio::buffer_copy(boost::asio::buffer(header), con->read_buffer.data());
		con->read_buffer.consume(HEADER_SIZE);

		if (std::memcmp(header.data(), magic.data(), MAGIC_SIZE) != 0)
		{
			LOG_ERROR("Invalid magic bytes from peer");
			remove_connection(con);
			return;
		}

		uint32_t payload_length = 0;
		std::memcpy(&payload_length, header.data() + MAGIC_SIZE, sizeof(payload_length));

		if (payload_length == 0 || payload_length > MAX_PAYLOAD_SIZE)
		{
			LOG_ERROR("Invalid payload length {}", payload_length);
			remove_connection(con);
			return;
		}

		std::array<uint8_t, CHECKSUM_SIZE> expected_checksum{};
		std::memcpy(expected_checksum.data(), header.data() + MAGIC_SIZE + sizeof(payload_length), CHECKSUM_SIZE);

		do_async_read_payload(con, payload_length, expected_checksum);
	}
	else
	{
		if (err != boost::asio::error::eof && err != boost::asio::error::shut_down && err !=
			boost::asio::error::connection_reset && err != boost::asio::error::operation_aborted)
			LOG_ERROR(err.message());

		remove_connection(con);
	}
}

void NetClient::do_async_read_payload(std::shared_ptr<Connection> con, uint32_t payload_length,
	std::array<uint8_t, CHECKSUM_SIZE> expected_checksum)
{
	auto handler = boost::bind(&NetClient::handle_read_payload, con, payload_length, expected_checksum,
		boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
	boost::asio::async_read(con->socket, con->read_buffer,
		boost::asio::transfer_exactly(payload_length), handler);
}

void NetClient::handle_read_payload(std::shared_ptr<Connection> con, uint32_t payload_length,
	std::array<uint8_t, CHECKSUM_SIZE> expected_checksum,
	const boost::system::error_code& err, [[maybe_unused]] size_t bytes_transferred)
{
	if (!err)
	{
		if (con->read_buffer.size() >= payload_length)
		{
			BinaryBuffer buffer;
			buffer.grow_to(payload_length);
			boost::asio::buffer_copy(boost::asio::buffer(buffer.get_writable_buffer()),
				con->read_buffer.data());
			con->read_buffer.consume(payload_length);

			const auto hash = SHA256::double_hash_binary(buffer.get_buffer());
			if (std::memcmp(hash.data(), expected_checksum.data(), CHECKSUM_SIZE) != 0)
			{
				LOG_ERROR("Checksum mismatch, dropping message");
			}
			else
			{
				handle_msg(con, buffer);
			}
		}

		do_async_read_header(con);
	}
	else
	{
		if (err != boost::asio::error::eof && err != boost::asio::error::shut_down && err !=
			boost::asio::error::connection_reset && err != boost::asio::error::operation_aborted)
			LOG_ERROR(err.message());

		remove_connection(con);
	}
}

void NetClient::handle_msg(const std::shared_ptr<Connection>& con, BinaryBuffer& msg_buffer)
{
	OpcodeType opcode = 0;
	if (!msg_buffer.read(opcode))
	{
		LOG_ERROR("No opcode");

		return;
	}
	auto opcode2 = static_cast<Opcode>(opcode);

	std::unique_ptr<IMsg> msg;
	switch (opcode2)
	{
		case Opcode::BlockInfoMsg:
		{
			msg = std::make_unique<BlockInfoMsg>();

			break;
		}
		case Opcode::GetActiveChainMsg:
		{
			msg = std::make_unique<GetActiveChainMsg>();

			break;
		}
		case Opcode::GetBlockMsg:
		{
			msg = std::make_unique<GetBlockMsg>();

			break;
		}
		case Opcode::GetMempoolMsg:
		{
			msg = std::make_unique<GetMempoolMsg>();

			break;
		}
		case Opcode::GetUTXOsMsg:
		{
			msg = std::make_unique<GetUTXOsMsg>();

			break;
		}
		case Opcode::InvMsg:
		{
			msg = std::make_unique<InvMsg>();

			break;
		}
		case Opcode::PeerAddMsg:
		{
			msg = std::make_unique<PeerAddMsg>();

			break;
		}
		case Opcode::PeerHelloMsg:
		{
			msg = std::make_unique<PeerHelloMsg>();

			break;
		}
		case Opcode::SendActiveChainMsg:
		{
			msg = std::make_unique<SendActiveChainMsg>();

			break;
		}
		case Opcode::SendMempoolMsg:
		{
			msg = std::make_unique<SendMempoolMsg>();

			break;
		}
		case Opcode::SendUTXOsMsg:
		{
			msg = std::make_unique<SendUTXOsMsg>();

			break;
		}
		case Opcode::TxInfoMsg:
		{
			msg = std::make_unique<TxInfoMsg>();

			break;
		}
		default:
		{
			LOG_ERROR("Unknown opcode {}", static_cast<OpcodeType>(opcode2));

			return;
		}
	}

	if (!msg->deserialize(msg_buffer))
	{
		LOG_ERROR("Unable to deserialize opcode {}", static_cast<OpcodeType>(opcode2));

		return;
	}
	msg->handle(con);
}

BinaryBuffer NetClient::prepare_send_buffer(const IMsg& msg)
{
	const auto serialized = msg.serialize();
	const auto& serialized_msg = serialized.get_buffer();
	const auto opcode = static_cast<OpcodeType>(msg.get_opcode());
	const auto payload_length = static_cast<uint32_t>(sizeof(opcode) + serialized_msg.size());

	std::vector<uint8_t> payload;
	payload.reserve(payload_length);
	payload.push_back(opcode);
	payload.insert(payload.end(), serialized_msg.begin(), serialized_msg.end());

	const auto hash = SHA256::double_hash_binary(payload);

	BinaryBuffer msg_buffer;
	msg_buffer.reserve(static_cast<uint32_t>(HEADER_SIZE + payload_length));
	msg_buffer.write_raw(std::vector<uint8_t>(magic.begin(), magic.end()));
	msg_buffer.write(payload_length);
	msg_buffer.write_raw(std::vector<uint8_t>(hash.begin(), hash.begin() + CHECKSUM_SIZE));
	msg_buffer.write_raw(payload);

	return msg_buffer;
}

void NetClient::write(const std::shared_ptr<Connection>& con, const BinaryBuffer& msg_buffer)
{
	boost::system::error_code err;
	{
		std::scoped_lock lock(con->write_mutex);

		boost::asio::write(con->socket, boost::asio::buffer(msg_buffer.get_buffer()), err);
	}
	if (err)
	{
		if (err != boost::asio::error::shut_down && err != boost::asio::error::connection_reset && err !=
			boost::asio::error::operation_aborted)
			LOG_ERROR(err.message());

		remove_connection(con);
	}
}

void NetClient::remove_connection(const std::shared_ptr<Connection>& con)
{
	std::scoped_lock lock(connections_mutex);

	const auto vec_it = std::find(connections.begin(), connections.end(), con);
	if (vec_it != connections.end())
	{
		auto& soc = con->socket;

		if (soc.is_open())
		{
			boost::system::error_code ec;
			const auto endpoint = soc.remote_endpoint(ec);
			if (!ec)
			{
				LOG_TRACE("Peer {}:{} disconnected", endpoint.address().to_string(),
					endpoint.port());
			}

			soc.shutdown(boost::asio::socket_base::shutdown_both, ec);
			soc.close(ec);
		}
		if (con->node_type & NodeType::Miner)
		{
			const auto vec_it2 = std::find(miner_connections.begin(), miner_connections.end(), con);
			if (vec_it2 != miner_connections.end())
			{
				miner_connections.erase(vec_it2);
			}
		}
		connections.erase(vec_it);
	}
}
