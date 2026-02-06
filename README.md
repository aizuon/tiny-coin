# tiny-coin

A minimal Bitcoin implementation in C++23, built for learning how Proof of Work cryptocurrencies function under the hood — from mining and transaction validation to peer-to-peer networking.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://isocpp.org/)
[![CMake 3.30+](https://img.shields.io/badge/CMake-3.30%2B-blue.svg)](https://cmake.org/)

## Features

### Core

- **Proof of Work mining** — multithreaded block mining with automatic difficulty adjustment (retargets every 144 blocks)
- **UTXO model** — faithful Unspent Transaction Output tracking as in Bitcoin
- **Transaction engine** — creation, validation, and spending with full ECDSA signature verification
- **Merkle tree** — block transaction integrity verification
- **Chain management** — side-branch tracking, fork detection, and automatic chain reorganisation
- **Mempool** — pending transaction pool with fee-based ordering
- **Chain persistence** — serialises the active chain to disk and restores on startup

### Cryptography

- **SHA-256** double hashing for block and transaction IDs
- **RIPEMD-160** for address derivation
- **ECDSA** (secp256k1) key generation and transaction signing via OpenSSL
- **Base58** encoding for human-readable wallet addresses

### Networking

- **Peer-to-peer** TCP protocol over Boost.Asio
- **Initial block download** — new nodes sync the full chain from peers
- **Message types** — block announcements, transaction broadcasts, mempool/UTXO queries, peer discovery

### Wallet

- Key generation and wallet file management
- Balance queries from the UTXO set
- Interactive CLI for sending coins and checking transaction status

## Network Parameters

| Parameter                  | Value              |
| -------------------------- | ------------------ |
| Block target interval      | 10 minutes         |
| Difficulty retarget period | 1 day (144 blocks) |
| Initial difficulty         | 24 bits            |
| Max block size             | 1 MB               |
| Max supply                 | 21 000 000 coins   |
| Coinbase maturity          | 2 blocks           |
| Subsidy halving interval   | 210 000 blocks     |

## Project Structure

```
tiny-coin/
├── tiny-lib/               Core static library
│   ├── core/               Block, Chain, Tx, Mempool, UTXO types
│   ├── crypto/             SHA-256, RIPEMD-160, ECDSA, Base58
│   ├── mining/             Proof of Work solver, Merkle tree
│   ├── net/                P2P message protocol & TCP networking
│   ├── util/               Binary serialisation, logging, helpers
│   └── wallet/             Wallet I/O and node configuration
├── tiny-sandbox/           CLI node application
├── tiny-test/              Unit tests (Google Test)
└── cmake/                  Toolchain & preset files
```

## Prerequisites

| Dependency   | Version                                         |
| ------------ | ----------------------------------------------- |
| CMake        | 3.30+                                           |
| C++ compiler | C++23 support (MSVC 19.44+, GCC 15+, Clang 20+) |
| vcpkg        | latest                                          |

### vcpkg packages

Install the following packages via vcpkg:

```
openssl  boost  fmt  spdlog  gtest
```

Set the **`VCPKG_ROOT`** environment variable to your vcpkg installation path before configuring.

> **Note:** On macOS/Linux you can install packages without specifying a triplet. On Windows you must install them for the `x64-windows-static` triplet (e.g. `vcpkg install openssl:x64-windows-static`).

## Building

The project uses **CMake Presets** for a reproducible build:

```bash
# Configure (pick one)
cmake --preset debug
cmake --preset release

# Build
cmake --build --preset debug      # or: --preset release

# Run tests
ctest --test-dir build/debug -C Debug --output-on-failure
```



## Quick Start

### 1. Start a miner node

```
$ ./tiny-sandbox --port 9901 --node_type miner --wallet miner.dat

[ 17:49:24 ] [ tc ] Generating new wallet miner.dat
[ 17:49:26 ] [ tc ] Your address is 172eJPtmt5wMq71bnT1fS55FYLHw1syhWB
[ 17:49:28 ] [ tc ] Load chain failed, starting from genesis
```

The miner will begin solving blocks immediately. Wait for a few blocks to be mined before continuing.

### 2. Start a wallet node

```
$ ./tiny-sandbox --port 9902 --node_type wallet --wallet wallet.dat

[ 17:49:48 ] [ tc ] Your address is 1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa
```

The wallet node syncs the chain from the miner and drops into an interactive shell.

### 3. Interactive commands

| Command                                              | Description                                     |
| ---------------------------------------------------- | ----------------------------------------------- |
| `address <wallet_file>`                              | Show the address for a wallet file              |
| `balance [<address>]`                                | Show balance (own address if omitted)           |
| `send <address> <amount> [fee_per_byte] [lock_time]` | Send coins (default fee: 100 coins/byte)        |
| `rbf <tx_id> <new_fee_per_byte>`                     | Replace a mempool transaction with a higher fee |
| `tx_status <tx_id>`                                  | Check whether a transaction is confirmed        |
| `exit` / `quit`                                      | Shut down the node                              |

**Example session:**

```
$ address receiver.dat
[ 17:50:57 ] [ tc ] Wallet receiver.dat belongs to address 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi

$ send 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi 50000
[ 17:51:23 ] [ tc ] Built transaction a5328f...fa9eef with 100 coins/byte fee
[ 17:51:23 ] [ tc ] Built transaction a5328f...fa9eef, broadcasting

$ balance 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi
[ 17:53:11 ] [ tc ] Address 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi holds 50000 coins
```

## Tests

The test suite covers core serialisation, cryptographic primitives, chain validation, Merkle trees, message encoding, and wallet operations:

```bash
ctest --test-dir build/debug -C Debug --output-on-failure
```

Test sources live under `tiny-test/src/`:

| File                      | Covers                                           |
| ------------------------- | ------------------------------------------------ |
| `binary_buffer_tests.cpp` | Binary serialisation round-trips                 |
| `block_chain_tests.cpp`   | Block creation, chain connect/disconnect, reorgs |
| `crypto_tests.cpp`        | SHA-256, RIPEMD-160, ECDSA sign/verify           |
| `merkle_tree_tests.cpp`   | Merkle root computation                          |
| `msg_tests.cpp`           | Network message serialise/deserialise            |
| `rbf_tests.cpp`           | Replace-by-fee signaling and mempool replacement |
| `serialization_tests.cpp` | Tx/Block binary encoding                         |
| `utils_tests.cpp`         | Utility helpers                                  |
| `wallet_tests.cpp`        | Wallet key generation and address derivation     |

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                      tiny-sandbox (CLI)                     │
├─────────────────────────────────────────────────────────────┤
│  Wallet          Mining (PoW)         Net Client            │
│  ┌──────────┐    ┌──────────────┐     ┌──────────────────┐  │
│  │ Key mgmt │    │ Assemble blk │     │ TCP peer-to-peer │  │
│  │ Send tx  │    │ Solve nonce  │     │ Msg broadcast    │  │
│  │ Balance  │    │ Difficulty   │     │ Initial block DL │  │
│  └──────────┘    └──────────────┘     └──────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                        Core Layer                           │
│  Chain ─ Block ─ Tx ─ TxIn/TxOut ─ UTXO ─ Mempool           │
├─────────────────────────────────────────────────────────────┤
│                      Crypto Layer                           │
│  SHA-256 ─ RIPEMD-160 ─ ECDSA (secp256k1) ─ Base58          │
├─────────────────────────────────────────────────────────────┤
│                     Utility Layer                           │
│  BinaryBuffer ─ Logging (spdlog) ─ Random ─ uint256_t       │
└─────────────────────────────────────────────────────────────┘
```

## Roadmap

- [ ] Orphan block handling
- [ ] Chainwork-based best-chain selection
- [ ] Full node type (combined miner + wallet in a single node)

## License

This project is licensed under the [MIT License](LICENSE).
