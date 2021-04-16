#include "pch.hpp"

#include <fstream>
#include <iterator>

#include "Wallet.hpp"
#include "RIPEMD160.hpp"
#include "SHA256.hpp"
#include "Base58.hpp"
#include "ECDSA.hpp"

std::string Wallet::PubKeyToAddress(const std::vector<uint8_t>& pubKey)
{
    auto sha256 = SHA256::HashBinary(pubKey);

    auto ripe = RIPEMD160::HashBinary(sha256);

    ripe.insert(ripe.begin(), 0x00);

    auto sha256d = SHA256::DoubleHashBinary(ripe);

    std::vector<uint8_t> checksum(sha256d.begin(), sha256d.begin() + 4);

    ripe.insert(ripe.end(), checksum.begin(), checksum.end());

    return PubKeyHashVersion + Base58::Encode(ripe);
}

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::string> Wallet::InitWallet()
{
    std::ifstream wallet_in(WalletPath, std::ios::binary);
    if (wallet_in.good())
    {
        std::vector<uint8_t> privKey(std::istreambuf_iterator<char>(wallet_in), {});
        auto pubKey = ECDSA::GetPubKeyFromPrivKey(privKey);
        auto address = PubKeyToAddress(pubKey);

        return { privKey, pubKey, address };
    }
    else
    {
        auto [privKey, pubKey] = ECDSA::Generate();
        auto address = PubKeyToAddress(pubKey);

        std::ofstream wallet_out(WalletPath, std::ios::binary);
        wallet_out.write((const char*)privKey.data(), privKey.size());
        wallet_out.flush();
        wallet_out.close();

        return { privKey, pubKey, address };
    }
}
