#include "pch.hpp"

#include <cstdint>
#include <cstdlib>
#include <future>
#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "../tiny-lib/Log.hpp"
#include "../tiny-lib/Crypto.hpp"
#include "../tiny-lib/NetClient.hpp"
#include "../tiny-lib/NodeConfig.hpp"
#include "../tiny-lib/PoW.hpp"
#include "../tiny-lib/Wallet.hpp"

namespace po = boost::program_options;

void atexit_handler()
{
	Crypto::CleanUp();
	Log::StopLog();
}

int main(int argc, char** argv)
{
	Log::StartLog();
	Crypto::Init();
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

	if (vm["node_type"].as<std::string>() == "miner")
	{
		NodeConfig::Type = NodeType::Miner;
	}
	else if (vm["node_type"].as<std::string>() == "wallet")
	{
		NodeConfig::Type = NodeType::Wallet;
	}
	else if (vm["node_type"].as<std::string>() == "full")
	{
		NodeConfig::Type = NodeType::Full; //TODO: implement full node
	}
	else
	{
		LOG_ERROR("Invalid node type");

		return EXIT_FAILURE;
	}

	const auto [priv_key, pub_key, address] = vm.contains("wallet")
		                                        ? Wallet::InitWallet(vm["wallet"].as<std::string>())
		                                        : Wallet::InitWallet();
	const auto port = vm["port"].as<uint16_t>();
	NetClient::ListenAsync(port);
	NetClient::RunAsync();

	std::vector<std::future<void>> pending_connections;
	for (const auto& [k, v] : NetClient::InitialPeers)
	{
		if (v == port)
		{
			continue;
		}

		pending_connections.emplace_back(std::async(std::launch::async, &NetClient::Connect, k, v));
	}
	for (const auto& pending_connection : pending_connections)
		pending_connection.wait();

	if (NodeConfig::Type == NodeType::Miner)
	{
		PoW::MineForever();
	}
	else
	{
		std::string wallet_address = "address ";
		std::string balance_address = "balance ";
		std::string balance_own = "balance";
		std::string send = "send ";
		std::string tx_status = "tx_status ";
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
				Wallet::PrintWalletAddress(command);
			}
			else if (command.starts_with(balance_address))
			{
				command.erase(0, balance_address.length());
				Wallet::PrintBalance(command);
			}
			else if (command.starts_with(balance_own))
			{
				Wallet::PrintBalance(address);
			}
			else if (command.starts_with(send))
			{
				command.erase(0, send.length());
				std::vector<std::string> send_args;
				boost::split(send_args, command, boost::is_any_of(" "));
				if (send_args.size() != 2 && send_args.size() != 3)
				{
					LOG_ERROR(
						"Send command requires 2 arguments, receiver address, send value and optionally fee per byte");

					continue;
				}
				const auto& send_address = send_args[0];
				const auto& send_value = send_args[1];
				if (send_args.size() == 3)
				{
					const auto& send_fee = send_args[2];
					Wallet::SendValue(std::stoull(send_value), std::stoull(send_fee), send_address, priv_key);
				}
				else
				{
					Wallet::SendValue(std::stoull(send_value), 100, send_address, priv_key);
				}
			}
			else if (command.starts_with(tx_status))
			{
				command.erase(0, tx_status.length());
				Wallet::PrintTxStatus(command);
			}
			else
			{
				LOG_ERROR("Unknown command");
			}
		}
	}

	NetClient::Stop();

	return EXIT_SUCCESS;
}
