# Tinycoin

## What is it?

Tinycoin is a minimal implementation of Bitcoin in C++. It is not designed to be used as a Bitcoin node, but rather to learn how Proof of Work cryptocurrencies function and provide a small example project. 

## Building

Windows 10 with Visual Studio 2019 for build environment and vcpkg for library management is strongly suggested. The project depends on the following x64 and x86 static libraries: 

- openssl
- boost
- tbb
- fmt
- spdlog

## Quick start

- Build the project
- Run a miner instance of sandbox
```tiny-sandbox.exe --port 9901 --walet miner.dat --mine```
- Wait for a few blocks to go by
- Generate an empty wallet
```tiny-sandbox.exe --port 9902 --wallet receiver.dat --balance```
- Send coins from miner wallet to empty wallet
```tiny-sandbox.exe --port 9902 --wallet miner.dat --send_address {receiver's address} --send_value 50```
- Wait for transaction to get mined in a block
- Check balance of reciever wallet
```tiny-sandbox.exe --port 9902 --wallet receiver.dat --balance```

## TODO

- Replace by fee
- Transaction locking
- Mempool sorting by fee
- Orphan blocks
- Chainwork
- Peer discovery