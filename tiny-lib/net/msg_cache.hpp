#pragma once
#include <memory>

#include "net/send_active_chain_msg.hpp"
#include "net/send_mempool_msg.hpp"
#include "net/send_utxos_msg.hpp"

class MsgCache
{
public:
	static std::shared_ptr<SendActiveChainMsg> send_active_chain_msg;
	static std::shared_ptr<SendMempoolMsg> send_mempool_msg;
	static std::shared_ptr<SendUTXOsMsg> send_utxos_msg;

	static constexpr uint16_t MAX_MSG_AWAIT_TIME_IN_SECS = 60;
};
