# tiny-coin

A minimal Bitcoin implementation in C++23, built for learning how Proof of Work cryptocurrencies function under the hood — from mining and transaction validation to peer-to-peer networking.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://isocpp.org/)
[![CMake 3.30+](https://img.shields.io/badge/CMake-3.30%2B-blue.svg)](https://cmake.org/)

## Features

### Core

- **Proof of Work mining** — multithreaded block mining with automatic difficulty adjustment (retargets every 144 blocks); pluggable backend system with CUDA GPU acceleration on NVIDIA hardware, Metal GPU acceleration on Apple Silicon, and automatic CPU fallback
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
| Max block size             | 1 000 000 bytes    |
| Max supply                 | 21 000 000 coins   |
| Coin precision             | 10^8 units / coin  |
| Coinbase maturity          | 2 blocks           |
| Subsidy halving interval   | 210 000 blocks     |
| Max future block time      | 2 hours            |
| Locktime threshold         | 500 000 000        |
| Max orphan blocks          | 50                 |
| Orphan block expiry        | 1 hour             |
| Incremental relay fee      | 1 000 units        |
| Max mempool size           | 300 MB             |
| Max ancestor count         | 25                 |
| Max descendant count       | 25                 |
| Dust threshold             | 546 units          |
| Mempool tx expiry          | 14 days            |

## Project Structure

```
tiny-coin/
├── tiny-lib/               Core static library
│   ├── core/               Block, Chain, Tx, Mempool, UTXO types
│   ├── crypto/             SHA-256, RIPEMD-160, ECDSA, Base58, HMAC-SHA512
│   ├── mining/             PoW solver, Merkle tree, fee estimator, mining backends
│   │   ├── cuda/           NVIDIA CUDA GPU compute kernel & backend
│   │   └── metal/          Apple Metal GPU compute shader & backend
│   ├── net/                P2P message protocol, opcodes & TCP networking
│   ├── util/               Binary serialisation, logging, helpers
│   └── wallet/             HD wallet, key derivation, node configuration
├── tiny-sandbox/           CLI node application
├── tiny-test/              Unit tests (Google Test)
├── tiny-bench/             Mining benchmark (CPU vs GPU)
└── cmake/                  Toolchain, presets & build scripts
```

## Prerequisites

| Dependency   | Version                                         |
| ------------ | ----------------------------------------------- |
| CMake        | 3.30+                                           |
| C++ compiler | C++23 support (MSVC 19.44+, GCC 15+, Clang 20+) |
| Python       | 3.x (build-time shader embedding)               |
| vcpkg        | latest                                          |
| CUDA Toolkit | 12.x+ (optional — NVIDIA GPU mining)            |

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

# Run mining benchmark (CPU vs Metal/CUDA GPU)
./build/debug/tiny-bench/tiny-bench
```

On Apple Silicon the build automatically enables Metal GPU mining. On Windows/Linux, if the **`CUDA_PATH`** environment variable is set and `nvcc` is found, CUDA GPU mining is enabled automatically. On other platforms or when no GPU toolkit is available, the CPU backend is used.

### GPU Mining

- **NVIDIA (CUDA):** Set the `CUDA_PATH` environment variable to your CUDA Toolkit installation (e.g. `C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.x`). The build will detect `nvcc` and compile the CUDA mining kernel automatically.
- **Apple Silicon (Metal):** No extra setup needed — the Metal backend is enabled automatically on macOS.



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

| File                       | Covers                                                               |
| -------------------------- | -------------------------------------------------------------------- |
| `binary_buffer_tests.cpp`  | Binary serialisation round-trips                                     |
| `block_chain_tests.cpp`    | Block creation, chain connect/disconnect, reorgs, orphan blocks, RBF |
| `cpfp_tests.cpp`           | Child-Pays-For-Parent package selection and block assembly           |
| `crypto_tests.cpp`         | SHA-256, RIPEMD-160, ECDSA sign/verify                               |
| `fee_estimator_tests.cpp`  | Fee rate estimation, priority buckets, mempool percentiles           |
| `hd_wallet_tests.cpp`      | BIP32 HD key derivation, seed round-trips, address generation        |
| `mempool_policy_tests.cpp` | Dust filtering, duplicate rejection, mempool UTXO lookup             |
| `merkle_tree_tests.cpp`    | Merkle root computation                                              |
| `msg_tests.cpp`            | Network message serialise/deserialise                                |
| `serialization_tests.cpp`  | Tx/Block binary encoding                                             |
| `utils_tests.cpp`          | Utility helpers                                                      |
| `wallet_tests.cpp`         | Wallet key generation and address derivation                         |

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
│                     Mining Backends                         │
│  CPU (boost::thread) ─ CUDA GPU (NVIDIA) ─ Metal GPU (Apple)│
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

- [ ] Full node type (combined miner + wallet in a single node)
- [ ] **OpenCL mining backend** — cross-platform GPU mining fallback

### High Priority — Core Consensus & Security

- [ ] **Bitcoin Script interpreter with P2SH** — replace hardcoded P2PKH validation with a stack-based script engine (`OP_DUP OP_HASH160 … OP_EQUALVERIFY OP_CHECKSIG`); extend with Pay-to-Script-Hash for multisig and complex contracts
- [ ] **Signature standards (BIP62/BIP66 & SIGHASH)** — require low-S values and strict DER encoding to prevent malleability; support `SIGHASH_ALL`, `SIGHASH_NONE`, `SIGHASH_SINGLE`, and `ANYONECANPAY` (currently only implicit `SIGHASH_ALL`)
- [ ] **Block sigops limit** — count signature operations per block and enforce a maximum, preventing pathologically expensive blocks

### Medium Priority — Protocol & Storage

- [ ] **Headers-first sync** — download and validate block headers before fetching bodies, enabling parallel block downloads and faster initial sync (currently sequential full-block IBD with assume-valid optimisation)
- [ ] **Persistent UTXO database** — move the UTXO set from an in-memory map to a key-value store (e.g. LevelDB / SQLite) so the node scales to larger chains (UTXO map is rebuilt from disk-persisted chain on startup)
- [ ] **Transaction index** — maintain a persistent txid → block position index for O(1) transaction lookups without full chain scans
- [ ] **Peer scoring & DoS protection** — track peer behaviour, score misbehaving nodes, and enforce banning and rate-limiting to prevent resource exhaustion (currently only magic-byte and checksum validation)
- [ ] **Merkle proofs & SPV verification** — expose proof-path extraction and verification from the existing Merkle tree so light clients can confirm transactions without downloading full blocks

### Lower Priority — Advanced Features

- [ ] **Segregated Witness (SegWit)** — separate witness data from the transaction ID to fix malleability and increase effective block capacity
- [ ] **Compact block relay (BIP152)** — send only short txid sketches instead of full blocks, reducing propagation latency
- [ ] **Compact block filters (BIP157/158)** — generate Golomb-coded set filters per block so light clients can determine relevant blocks without downloading full chain data
- [ ] **BIP39 mnemonic seed phrases** — derive HD wallet seeds from a human-readable word list for easier backup and recovery (BIP32/BIP44 key derivation is already implemented)
- [ ] **Watch-only wallet** — import xpubs or bare addresses to monitor balances and transaction history without exposing private keys

## License

This project is licensed under the [MIT License](LICENSE).
