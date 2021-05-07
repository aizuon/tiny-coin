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

- Run a miner node
  ```
  $ tiny-sandbox.exe --port 9901 --node_type miner --wallet miner.dat
  
  [ 17:17:23 ] [ tc ] Generating new wallet miner.dat
  [ 17:17:25 ] [ tc ] Your address is 172eJPtmt5wMq71bnT1fS55FYLHw1syhWB
  [ 17:17:26 ] [ tc ] Load chain failed, starting from genesis
  ```
- Wait for a few blocks to go by
  ```
  [ 17:17:27 ] [ tc ] Block found => 2 s, 835 KH/s, 000000fe05062373c95152a779915f25d75cfc519395a120d3bba466b88b9bc5, 422750
  [ 17:17:27 ] [ tc ] Connecting block 000000fe05062373c95152a779915f25d75cfc519395a120d3bba466b88b9bc5 to chain 0
  [ 17:17:27 ] [ tc ] Adding TxOutPoint f353379e1dce17f3964bd1e673eb268faabaf8e6a24cc866f46389f54d920b25 to UTXO map
  [ 17:17:27 ] [ tc ] Block accepted with height 1 and txs 1
  [ 17:17:27 ] [ tc ] Saving chain with 2 blocks
  ...
  ```
- Run a wallet node
  ```
  $ tiny-sandbox.exe --port 9902 --node_type wallet --wallet miner.dat
  
  [ 17:17:31 ] [ tc ] Your address is 172eJPtmt5wMq71bnT1fS55FYLHw1syhWB
  ```
- Generate an empty wallet
  ```
  $ address receiver.dat

  [ 17:21:06 ] [ tc ] receiver.dat belongs to address is 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi
  ```
- Send coins from miner wallet to empty wallet
  ```
  $ send 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi 50
  
  [ 17:21:52 ] [ tc ] Built transaction df02039631785fe23eb047bf8222f6cb89e767c6ee9e56c2e06d14ac0624c843, broadcasting
  ```
- Wait for transaction to get mined in a block
  ```
  ...
  [ 17:22:03 ] [ tc ] Added transaction df02039631785fe23eb047bf8222f6cb89e767c6ee9e56c2e06d14ac0624c843 to block 02f30dcb19d2fdda6a2e2dd556aacb22197f0d66170c37d5bc3bc479a7cd583f
  [ 17:22:28 ] [ tc ] Block found => 25 s, 1146 KH/s, 0000009d480d7d6fb74541c35553a4e030461d3ee890312c152d209f12fa9dfd, 9223372036861901087
  [ 17:22:28 ] [ tc ] Connecting block 0000009d480d7d6fb74541c35553a4e030461d3ee890312c152d209f12fa9dfd to chain 0
  [ 17:22:28 ] [ tc ] Adding TxOutPoint 1972eeeea6949504e763fd0c22de36091ffddb734b546db3336c78d169c1a294 to UTXO map
  [ 17:22:28 ] [ tc ] Adding TxOutPoint df02039631785fe23eb047bf8222f6cb89e767c6ee9e56c2e06d14ac0624c843 to UTXO map
  [ 17:22:28 ] [ tc ] Block accepted with height 23 and txs 2
  [ 17:22:28 ] [ tc ] Saving chain with 24 blocks
  ...
  ```
- Check balance of receiver wallet
  ```
  $ balance 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi

  [ 17:23:41 ] [ tc ] Address 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi holds 50 coins
  ```

## TODO

- Replace by fee
- Transaction locking
- Mempool sorting by fee
- Orphan blocks
- Chainwork
- Peer discovery
- Full node