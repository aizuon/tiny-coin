# tiny-coin

A minimal Bitcoin implementation in C++23, built for learning how Proof of Work cryptocurrencies function.

## Features

- Block mining with configurable difficulty (Proof of Work)
- Transaction creation, validation, and spending with ECDSA signatures
- UTXO (Unspent Transaction Output) model
- Merkle tree construction for block transaction verification
- Chain reorganization and side branch handling
- Peer-to-peer networking with initial block download
- Wallet management with key generation and balance tracking
- Mempool for pending transactions
- Chain persistence to disk

## Project Structure

```
tiny-lib/           Core library
  core/             Block, chain, transaction, mempool, UTXO types
  crypto/           SHA-256, RIPEMD-160, ECDSA, Base58 encoding
  mining/           Proof of Work solver, Merkle tree
  net/              P2P message protocol, networking
  util/             Binary serialization, logging, helpers
  wallet/           Wallet and node configuration
tiny-sandbox/       CLI application
tiny-test/          Unit tests (Google Test)
```

## Prerequisites

- **CMake** 3.30+
- **C++23** compiler (MSVC 19.44+, GCC 15+, Clang 20+)
- **vcpkg** with the following packages installed for `x64-windows-static`:
  - `openssl`
  - `boost`
  - `fmt`
  - `spdlog`
  - `gtest`

Set `VCPKG_ROOT` environment variable to your vcpkg installation path.

## Building

```bash
# Configure
cmake --preset debug      # or: cmake --preset release

# Build
cmake --build --preset debug

# Test
ctest --test-dir build/debug -C Debug --output-on-failure
```

## Quick Start

Run a miner node:

```
$ tiny-sandbox.exe --port 9901 --node_type miner --wallet miner.dat

[ 17:49:24 ] [ tc ] Generating new wallet miner.dat
[ 17:49:26 ] [ tc ] Your address is 172eJPtmt5wMq71bnT1fS55FYLHw1syhWB
[ 17:49:28 ] [ tc ] Load chain failed, starting from genesis
```

Wait for a few blocks to be mined, then run a wallet node:

```
$ tiny-sandbox.exe --port 9902 --node_type wallet --wallet miner.dat

[ 17:49:48 ] [ tc ] Your address is 172eJPtmt5wMq71bnT1fS55FYLHw1syhWB
```

Generate a receiver wallet and send coins:

```
$ address receiver.dat
[ 17:50:57 ] [ tc ] Wallet receiver.dat belongs to address 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi

$ send 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi 50000
[ 17:51:23 ] [ tc ] Built transaction a5328f...fa9eef with 100 coins/byte fee
[ 17:51:23 ] [ tc ] Built transaction a5328f...fa9eef, broadcasting
```

Check balance after the transaction is mined:

```
$ balance 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi
[ 17:53:11 ] [ tc ] Address 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi holds 50000 coins
```

## TODO

- Replace by fee
- Transaction locking
- Orphan blocks
- Chainwork
- Peer discovery
- Full node

## License

See [LICENSE](LICENSE) for details.