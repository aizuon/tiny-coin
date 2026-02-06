#include "net/msg_cache.hpp"

std::shared_ptr<SendActiveChainMsg> MsgCache::send_active_chain_msg;
std::shared_ptr<SendMempoolMsg> MsgCache::send_mempool_msg;
std::shared_ptr<SendUTXOsMsg> MsgCache::send_utxos_msg;
