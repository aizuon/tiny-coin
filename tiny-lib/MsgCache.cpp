#include "pch.hpp"

#include "MsgCache.hpp"

std::shared_ptr<SendActiveChainMsg> MsgCache::SendActiveChainMsg;
std::shared_ptr<SendMempoolMsg> MsgCache::SendMempoolMsg;
std::shared_ptr<SendUTXOsMsg> MsgCache::SendUTXOsMsg;
