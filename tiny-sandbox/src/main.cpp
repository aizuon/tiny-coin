#include <cstdlib>
#include <future>
#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "util/log.hpp"
#include "crypto/crypto.hpp"
#include "mining/fee_estimator.hpp"
#include "net/net_client.hpp"
#include "wallet/node_config.hpp"
#include "mining/pow.hpp"
#include "wallet/wallet.hpp"

namespace po = boost::program_options;

void atexit_handler()
{
	NetClient::stop();
	Crypto::cleanup();
	Log::stop_log();
}

int main(int argc, char** argv)
{
	Log::start_log();
	Crypto::init();
	if (std::atexit(atexit_handler) != 0)
		return EXIT_FAILURE;

	po::options_description desc("allowed options");
	desc.add_options()
		("node_type", po::value<std::string>(), "specify node type")
		("port", po::value<uint16_t>(), "port to listen on network connections")
		("wallet", po::value<std::string>(), "path to wallet");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (!vm.contains("port"))
	{
		LOG_ERROR("A port for network connection must be specified");

		return EXIT_FAILURE;
	}

	if (!vm.contains("node_type"))
	{
		LOG_ERROR("Node type must be specified");

		return EXIT_FAILURE;
	}

	const auto& node_type = vm["node_type"].as<std::string>();
	if (node_type == "miner")
	{
		NodeConfig::type = NodeType::Miner;
	}
	else if (node_type == "wallet")
	{
		NodeConfig::type = NodeType::Wallet;
	}
	else if (node_type == "full")
	{
		NodeConfig::type = NodeType::Full; //TODO: implement full node
	}
	else
	{
		LOG_ERROR("Invalid node type");

		return EXIT_FAILURE;
	}

	const auto [priv_key, pub_key, address] = vm.contains("wallet")
		? Wallet::init_wallet(vm["wallet"].as<std::string>())
		: Wallet::init_wallet();
	const auto port = vm["port"].as<uint16_t>();
	NetClient::listen_async(port);
	NetClient::run_async();

	std::vector<std::future<void>> pending_connections;
	for (const auto& [k, v] : NetClient::initial_peers)
	{
		if (v == port)
		{
			continue;
		}

		pending_connections.emplace_back(std::async(std::launch::async, &NetClient::connect, k, v));
	}
	for (const auto& pending_connection : pending_connections)
		pending_connection.wait();

	if (NodeConfig::type == NodeType::Miner)
	{
		PoW::mine_forever();
	}
	else
	{
		const std::string wallet_address = "address ";
		const std::string balance_address = "balance ";
		const std::string balance_own = "balance";
		const std::string send = "send ";
		const std::string rbf = "rbf ";
		const std::string tx_status = "tx_status ";
		const std::string fee_estimate = "fee_estimate";
		while (true)
		{
			std::string command;
			std::getline(std::cin, command);

			if (command == "exit" || command == "quit")
			{
				break;
			}

			if (command.starts_with(wallet_address))
			{
				command.erase(0, wallet_address.length());
				Wallet::print_wallet_address(command);
			}
			else if (command.starts_with(balance_address))
			{
				command.erase(0, balance_address.length());
				Wallet::print_balance(command);
			}
			else if (command.starts_with(balance_own))
			{
				Wallet::print_balance(address);
			}
			else if (command.starts_with(send))
			{
				command.erase(0, send.length());
				std::vector<std::string> send_args;
				boost::split(send_args, command, boost::is_any_of(" "));
				if (send_args.size() < 2 || send_args.size() > 4)
				{
					LOG_ERROR(
						"Send command requires: address, value, and optionally fee_per_byte and lock_time");

					continue;
				}
				const auto& send_address = send_args[0];
				const auto& send_value = send_args[1];
				try
				{
					const uint64_t fee = send_args.size() >= 3
						? std::stoull(send_args[2])
						: FeeEstimator::estimate_fee_rate(3);
					const int64_t lock_time = send_args.size() == 4 ? std::stoll(send_args[3]) : 0;
					LOG_INFO("Using fee rate: {} coins/byte", fee);
					Wallet::send_value(std::stoull(send_value), fee, send_address, priv_key, lock_time);
				}
				catch (const std::exception&)
				{
					LOG_ERROR("Invalid numeric argument for send command");
				}
			}
			else if (command.starts_with(rbf))
			{
				command.erase(0, rbf.length());
				std::vector<std::string> rbf_args;
				boost::split(rbf_args, command, boost::is_any_of(" "));
				if (rbf_args.size() != 2)
				{
					LOG_ERROR("rbf command requires 2 arguments: tx_id and new_fee_per_byte");

					continue;
				}
				try
				{
					Wallet::rbf_tx(rbf_args[0], std::stoull(rbf_args[1]), priv_key);
				}
				catch (const std::exception&)
				{
					LOG_ERROR("Invalid numeric argument for rbf command");
				}
			}
			else if (command.starts_with(tx_status))
			{
				command.erase(0, tx_status.length());
				Wallet::print_tx_status(command);
			}
			else if (command.starts_with(fee_estimate))
			{
				const uint64_t high = FeeEstimator::estimate_fee_rate(1);
				const uint64_t medium = FeeEstimator::estimate_fee_rate(3);
				const uint64_t low = FeeEstimator::estimate_fee_rate(6);
				LOG_INFO("Fee estimates (coins/byte): high={} (1 block), medium={} (3 blocks), low={} (6 blocks)",
					high, medium, low);
			}
			else
			{
				LOG_ERROR("Unknown command");
			}
		}
	}

	return EXIT_SUCCESS;
}
