#pragma once
#include <memory>

#include "P2P/SendActiveChainMsg.hpp"
#include "P2P/SendMempoolMsg.hpp"
#include "P2P/SendUTXOsMsg.hpp"

class MsgCache
{
public:
	static std::shared_ptr<SendActiveChainMsg> SendActiveChainMsg;
	static std::shared_ptr<SendMempoolMsg> SendMempoolMsg;
	static std::shared_ptr<SendUTXOsMsg> SendUTXOsMsg;

	static constexpr uint16_t MAX_MSG_AWAIT_TIME_IN_SECS = 60;
};
