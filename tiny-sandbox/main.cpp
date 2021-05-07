#include <cstdint>
#include <cstdlib>
#include <future>
#include <string>
#include <vector>
#include <boost/program_options.hpp>

#include "../tiny-lib/Log.hpp"
#include "../tiny-lib/NetClient.hpp"
#include "../tiny-lib/PoW.hpp"
#include "../tiny-lib/Wallet.hpp"
#include "../tiny-lib/NodeConfig.hpp"

namespace po = boost::program_options;

void atexit_handler()
{
	Log::StopLog();
}

int main(int ac, char** av)
{
	Log::StartLog();
	if (std::atexit(atexit_handler) != 0)
		return EXIT_FAILURE;

	po::options_description desc("allowed options");
	desc.add_options()
		("wallet", po::value<std::string>(), "path to wallet")
		("mine", "sets up a mining node")
		("port", po::value<uint16_t>(), "port to listen on network connections")
		("balance", "shows balance for this wallet")
		("send_address", po::value<std::string>(), "sends coins to address")
		("send_value", po::value<uint64_t>(), "sends coins to address")
		("tx_status", po::value<std::string>(), "shows status for transaction");

	po::variables_map vm;
	po::store(po::parse_command_line(ac, av, desc), vm);
	po::notify(vm);

	if (!vm.contains("port"))
	{
		LOG_ERROR("A port for network connection must be specified");

		Log::StopLog();

		return EXIT_FAILURE;
	}

	if (vm.contains("mine"))
	{
		NodeConfig::Type = NodeConfig::Type | Miner;
	}
	else
	{
		NodeConfig::Type = NodeConfig::Type | Wallet;
	}

	const auto [privKey, pubKey, address] = vm.contains("wallet")
		                                        ? Wallet::InitWallet(vm["wallet"].as<std::string>())
		                                        : Wallet::InitWallet();
	const auto port = vm["port"].as<uint16_t>();
	NetClient::ListenAsync(port);
	NetClient::RunAsync();

	std::vector<std::future<void>> pendingConnections;
	for (const auto& [k, v] : NetClient::InitialPeers)
	{
		if (v == port)
		{
			continue;
		}

		pendingConnections.emplace_back(std::async(std::launch::async, &NetClient::Connect, k, v));
	}
	for (const auto& pendingConnection : pendingConnections)
		pendingConnection.wait();

	if (NodeConfig::Type & Miner)
	{
		PoW::MineForever();
	}
	else
	{
		if (vm.contains("balance"))
		{
			Wallet::PrintBalance(address);
		}
		else if (vm.contains("tx_status"))
		{
			Wallet::PrintTxStatus(vm["tx_status"].as<std::string>());
		}
		else if (vm.contains("send_address") && vm.contains("send_value"))
		{
			Wallet::SendValue(vm["send_value"].as<uint64_t>(), vm["send_address"].as<std::string>(), privKey);
		}
	}

	NetClient::Stop();

	return EXIT_SUCCESS;
}
