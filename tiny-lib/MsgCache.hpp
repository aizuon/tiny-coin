#pragma once
#include <memory>

#include "SendActiveChainMsg.hpp"
#include "SendMempoolMsg.hpp"
#include "SendUTXOsMsg.hpp"

class MsgCache
{
public:
	static std::shared_ptr<SendActiveChainMsg> SendActiveChainMsg;
	static std::shared_ptr<SendMempoolMsg> SendMempoolMsg;
	static std::shared_ptr<SendUTXOsMsg> SendUTXOsMsg;

	static constexpr uint16_t MAX_MSG_AWAIT_TIME_IN_SECS = 60;
private:

};