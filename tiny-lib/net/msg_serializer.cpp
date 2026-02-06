#include "net/msg_serializer.hpp"

#include "util/binary_buffer.hpp"
#include "crypto/sha256.hpp"
#include "core/tx_out.hpp"
#include "core/tx_out_point.hpp"

std::vector<uint8_t> MsgSerializer::build_spend_msg(const std::shared_ptr<TxOutPoint>& to_spend,
	const std::vector<uint8_t>& pub_key, int32_t sequence,
	const std::vector<std::shared_ptr<TxOut>>& tx_outs)
{
	BinaryBuffer spend_message;
	spend_message.write_raw(to_spend->serialize().get_buffer());
	spend_message.write(sequence);
	spend_message.write(pub_key);
	for (const auto& tx_out : tx_outs)
	{
		spend_message.write_raw(tx_out->serialize().get_buffer());
	}

	const auto buffer = spend_message.get_buffer();

	auto hash = SHA256::double_hash_binary(buffer);

	return hash;
}
