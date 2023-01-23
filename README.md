# Tinycoin

## What is it?

Tinycoin is a minimal implementation of Bitcoin in C++. It is not designed to be used as a Bitcoin node, but rather to learn how Proof of Work cryptocurrencies function and provide a small example project. 

## Building

Windows 11 with Visual Studio 2022 for build environment and vcpkg for library management is strongly suggested. The project depends on the following x64 and x86 static libraries: 

- openssl
- boost
- fmt
- spdlog

## Quick start

- Run a miner node
  ```
  $ tiny-sandbox.exe --port 9901 --node_type miner --wallet miner.dat
  
  [ 17:49:24 ] [ tc ] Generating new wallet miner.dat
  [ 17:49:26 ] [ tc ] Your address is 172eJPtmt5wMq71bnT1fS55FYLHw1syhWB
  [ 17:49:28 ] [ tc ] Load chain failed, starting from genesis
  ```
- Wait for a few blocks to go by
  ```
  [ 17:49:28 ] [ tc ] Start mining block 0881dfd39dbc50325850c3992b5c18ba8c30520a4c4fdce588fb899f53d82e44 with 0 fees
  [ 17:49:47 ] [ tc ] Block found => 19 s, 607 KH/s, 000000245f1ad9e00ba03836316fa2a896b79b08f8e0b3c32fac4da5225ba877, 2877728
  [ 17:49:47 ] [ tc ] Connecting block 000000245f1ad9e00ba03836316fa2a896b79b08f8e0b3c32fac4da5225ba877 to chain 0
  [ 17:49:47 ] [ tc ] Adding TxOutPoint 72a1142476ef4c9241545fc02ca6ed48a337fbfd24171127fd1bcbad0a6fc44e to UTXO map
  [ 17:49:47 ] [ tc ] Block accepted at height 1 with 1 txs
  [ 17:49:47 ] [ tc ] Saving chain with 2 blocks
  ...
  ```
- Run a wallet node
  ```
  $ tiny-sandbox.exe --port 9902 --node_type wallet --wallet miner.dat
  
  [ 17:49:48 ] [ tc ] Your address is 172eJPtmt5wMq71bnT1fS55FYLHw1syhW
  ```
- Generate an empty wallet
  ```
  $ address receiver.dat

  [ 17:50:57 ] [ tc ] Wallet receiver.dat belongs to address 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi
  ```
- Send coins from miner wallet to empty wallet
  ```
  $ send 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi 50000
  
  [ 17:51:23 ] [ tc ] Built transaction a5328f5688ff6b39de25af6a7bab840cc4e1156a26dabb10e2d01b12cbfa9eef with 100 coins/byte fee
  [ 17:51:23 ] [ tc ] Built transaction a5328f5688ff6b39de25af6a7bab840cc4e1156a26dabb10e2d01b12cbfa9eef, broadcasting
  ```
- Wait for transaction to get mined in a block
  ```
  ...
  [ 17:51:23 ] [ tc ] Recieved transaction a5328f5688ff6b39de25af6a7bab840cc4e1156a26dabb10e2d01b12cbfa9eef from peer 127.0.0.1:62256
  [ 17:51:23 ] [ tc ] Transaction a5328f5688ff6b39de25af6a7bab840cc4e1156a26dabb10e2d01b12cbfa9eef added to mempool
  ...
  [ 17:51:44 ] [ tc ] Added transaction a5328f5688ff6b39de25af6a7bab840cc4e1156a26dabb10e2d01b12cbfa9eef to block 1f4fb281eb44989700c3e9aba535191e684657c06911e0f0aed1795f967c88df
  [ 17:51:44 ] [ tc ] Start mining block ff02250fc300e1b53c40528e9fba7f89df34e7f6cb4a896e8fde84872e23461b with 30000 fees
  [ 17:52:55 ] [ tc ] Block found => 71 s, 699 KH/s, 000000275c6044cf9f39de8710a1bdf799043d2d634dbcd1584d3591168b5123, 4611686018440151883
  [ 17:52:55 ] [ tc ] Connecting block 000000275c6044cf9f39de8710a1bdf799043d2d634dbcd1584d3591168b5123 to chain 0
  [ 17:52:55 ] [ tc ] Adding TxOutPoint bc1d12d2105f55527f359721e5b03e14a094037a36cbda70402a3543d939d3f4 to UTXO map
  [ 17:52:55 ] [ tc ] Adding TxOutPoint a5328f5688ff6b39de25af6a7bab840cc4e1156a26dabb10e2d01b12cbfa9eef to UTXO map
  [ 17:52:55 ] [ tc ] Adding TxOutPoint a5328f5688ff6b39de25af6a7bab840cc4e1156a26dabb10e2d01b12cbfa9eef to UTXO map
  [ 17:52:55 ] [ tc ] Block accepted at height 5 with 2 txs
  [ 17:52:55 ] [ tc ] Saving chain with 6 blocks
  ...
  ```
- Check balance of receiver wallet
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