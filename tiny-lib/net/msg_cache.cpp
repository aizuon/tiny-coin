#include "net/msg_cache.hpp"

std::shared_ptr<SendActiveChainMsg> MsgCache::send_active_chain_msg;
std::shared_ptr<SendMempoolMsg> MsgCache::send_mempool_msg;
std::shared_ptr<SendUTXOsMsg> MsgCache::send_utxos_msg;
std::mutex MsgCache::mutex;

void MsgCache::set_send_active_chain_msg(std::shared_ptr<SendActiveChainMsg> msg)
{
    std::scoped_lock lock(mutex);
    send_active_chain_msg = std::move(msg);
}

std::shared_ptr<SendActiveChainMsg> MsgCache::get_send_active_chain_msg()
{
    std::scoped_lock lock(mutex);
    return send_active_chain_msg;
}

void MsgCache::set_send_mempool_msg(std::shared_ptr<SendMempoolMsg> msg)
{
    std::scoped_lock lock(mutex);
    send_mempool_msg = std::move(msg);
}

std::shared_ptr<SendMempoolMsg> MsgCache::get_send_mempool_msg()
{
    std::scoped_lock lock(mutex);
    return send_mempool_msg;
}

void MsgCache::set_send_utxos_msg(std::shared_ptr<SendUTXOsMsg> msg)
{
    std::scoped_lock lock(mutex);
    send_utxos_msg = std::move(msg);
}

std::shared_ptr<SendUTXOsMsg> MsgCache::get_send_utxos_msg()
{
    std::scoped_lock lock(mutex);
    return send_utxos_msg;
}
