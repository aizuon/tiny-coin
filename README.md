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

- Run a miner instance
  ```
  $ tiny-sandbox.exe --port 9901 --wallet miner.dat --mine
  
  [ 03:04:45 ] [ tc ] Generating new wallet miner.dat
  [ 03:04:45 ] [ tc ] Your address is 172eJPtmt5wMq71bnT1fS55FYLHw1syhWB
  [ 03:04:47 ] [ tc ] Load chain failed, starting from genesis
  ```
- Wait for a few blocks to go by
  ```
  [ 03:04:47 ] [ tc ] Block found => 1 s, 726 KH/s, 0000001ec4d7580bf8b0459af362857d148112c8a38e5b039ca423558bf28ca3, 13835058055282350115
  [ 03:04:47 ] [ tc ] Connecting block 0000001ec4d7580bf8b0459af362857d148112c8a38e5b039ca423558bf28ca3 to chain 0
  [ 03:04:47 ] [ tc ] Adding TxOutPoint f353379e1dce17f3964bd1e673eb268faabaf8e6a24cc866f46389f54d920b25 to UTXO map
  [ 03:04:47 ] [ tc ] Block accepted with height 1 and txs 1
  [ 03:04:47 ] [ tc ] Saving chain with 2 blocks
  ...
  ```
- Generate an empty wallet
  ```
  $ tiny-sandbox.exe --port 9902 --wallet receiver.dat --balance
  
  [ 03:04:53 ] [ tc ] Generating new wallet receiver.dat
  [ 03:04:53 ] [ tc ] Your address is 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi
  [ 03:04:55 ] [ tc ] Address 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi holds 0 coins
  ```
- Send coins from miner wallet to empty wallet
  ```
  $ tiny-sandbox.exe --port 9902 --wallet miner.dat --send_address 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi --send_value 50
  
  [ 03:05:27 ] [ tc ] Built transaction bc74ea19c1eb5fd5d5571d2926f7f247e8c54844085aeb4902dddd3c587d934b, broadcasting
  ```
- Wait for transaction to get mined in a block
  ```
  [ 03:05:33 ] [ tc ] Added transaction bc74ea19c1eb5fd5d5571d2926f7f247e8c54844085aeb4902dddd3c587d934b to block 3d202ec5dda4e5bad64c52d29cb03d9342c9eac9062503613665fb606da15e51
  [ 03:05:50 ] [ tc ] Block found => 17 s, 1177 KH/s, 00000079f33dc7c0b50d51a425b8eae0c3bf406f71094b33a016aee05fdf26c7, 13835058055287314157
  [ 03:05:50 ] [ tc ] Connecting block 00000079f33dc7c0b50d51a425b8eae0c3bf406f71094b33a016aee05fdf26c7 to chain 0
  [ 03:05:50 ] [ tc ] Adding TxOutPoint e08baa6462a7a406879248f914cf8bb5a1758a73129f07f82aea7209769a754e to UTXO map
  [ 03:05:50 ] [ tc ] Adding TxOutPoint bc74ea19c1eb5fd5d5571d2926f7f247e8c54844085aeb4902dddd3c587d934b to UTXO map
  [ 03:05:50 ] [ tc ] Block accepted with height 8 and txs 2
  [ 03:05:50 ] [ tc ] Saving chain with 9 blocks
  ```
- Check balance of reciever wallet
  ```
  $ tiny-sandbox.exe --port 9902 --wallet receiver.dat --balance
  
  [ 03:05:54 ] [ tc ] Address 16KHYopvjuY3mVLtEPHDLTzemWbPV6agoi holds 50 coins
  ```

## TODO

- Replace by fee
- Transaction locking
- Mempool sorting by fee
- Orphan blocks
- Chainwork
- Peer discovery