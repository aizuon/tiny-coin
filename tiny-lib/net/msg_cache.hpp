#pragma once
#include <memory>
#include <mutex>

#include "net/send_active_chain_msg.hpp"
#include "net/send_mempool_msg.hpp"
#include "net/send_utxos_msg.hpp"

class MsgCache
{
public:
	static void set_send_active_chain_msg(std::shared_ptr<SendActiveChainMsg> msg);
	static std::shared_ptr<SendActiveChainMsg> get_send_active_chain_msg();

	static void set_send_mempool_msg(std::shared_ptr<SendMempoolMsg> msg);
	static std::shared_ptr<SendMempoolMsg> get_send_mempool_msg();

	static void set_send_utxos_msg(std::shared_ptr<SendUTXOsMsg> msg);
	static std::shared_ptr<SendUTXOsMsg> get_send_utxos_msg();

	static constexpr uint16_t MAX_MSG_AWAIT_TIME_IN_SECS = 60;

private:
	static std::shared_ptr<SendActiveChainMsg> send_active_chain_msg;
	static std::shared_ptr<SendMempoolMsg> send_mempool_msg;
	static std::shared_ptr<SendUTXOsMsg> send_utxos_msg;

	static std::mutex mutex;
};
