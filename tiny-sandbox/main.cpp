#include <cstdint>
#include <atomic>
#include <string>
#include <tuple>
#include <thread>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")

#include "../tiny-lib/Log.hpp"
#include "../tiny-lib/NetClient.hpp"
#include "../tiny-lib/PoW.hpp"
#include "../tiny-lib/Wallet.hpp"

int main(int ac, char** av)
{
	Log::StartLog();

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

		return 0;
	}
	auto [privKey, pubKey, address] = vm.contains("wallet") ? Wallet::InitWallet(vm["wallet"].as<std::string>()) : Wallet::InitWallet();
	auto port = vm["port"].as<uint16_t>();
	NetClient::ListenAsync(port);
	std::thread io_thread(
		[]()
		{
			NetClient::Run();
		});

	for (const auto& [k, v] : NetClient::InitialPeers)
	{
		if (v == port)
		{
			continue;
		}

		NetClient::Connect(k, v);
	}

	if (vm.contains("mine"))
	{
		PoW::MineForever();
	}
	else if (vm.contains("balance"))
	{
		Wallet::GetBalance(address);
	}
	else if (vm.contains("tx_status"))
	{
		Wallet::PrintTxStatus(vm["tx_status"].as<std::string>());
	}
	else if (vm.contains("send_address") && vm.contains("send_value"))
	{
		Wallet::SendValue(vm["send_value"].as<uint64_t>(), vm["send_address"].as<std::string>(), privKey);
	}

	NetClient::Stop();
	io_thread.join();

	Log::StopLog();

	return 0;
}