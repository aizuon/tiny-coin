#ifdef TINY_COIN_CUDA

#if defined(__INTELLISENSE__) && !defined(__CUDACC__)
#define __device__
#define __global__
#define __constant__
#define __forceinline__
#define __CUDACC__
#define __NV_NO_HOST_COMPILER_CHECK
struct { unsigned int x, y, z; } threadIdx, blockIdx, blockDim, gridDim;
int atomicAdd(int*, int);
unsigned int atomicAdd(unsigned int*, unsigned int);
unsigned int atomicCAS(unsigned int*, unsigned int, unsigned int);
template <typename T>
void __launch_kernel(T, ...);
#define KERNEL_LAUNCH(kernel, grid, block, ...) kernel(__VA_ARGS__)
#else
#define KERNEL_LAUNCH(kernel, grid, block, ...) kernel<<<grid, block>>>(__VA_ARGS__)
#endif

#include "mining/cuda/cuda_mining_backend.hpp"

#include <cstring>

#include <cuda_runtime.h>

#include "util/log.hpp"

__constant__ static uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

__device__ __forceinline__ uint32_t rotr(uint32_t x, uint32_t n)
{
    return (x >> n) | (x << (32 - n));
}

__device__ __forceinline__ uint32_t ch(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (~x & z);
}

__device__ __forceinline__ uint32_t maj(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

__device__ __forceinline__ uint32_t sigma0(uint32_t x)
{
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

__device__ __forceinline__ uint32_t sigma1(uint32_t x)
{
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

__device__ __forceinline__ uint32_t gamma0(uint32_t x)
{
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

__device__ __forceinline__ uint32_t gamma1(uint32_t x)
{
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

struct Sha256State
{
    uint32_t h[8];
};

__device__ Sha256State sha256_init()
{
    Sha256State s;
    s.h[0] = 0x6a09e667;
    s.h[1] = 0xbb67ae85;
    s.h[2] = 0x3c6ef372;
    s.h[3] = 0xa54ff53a;
    s.h[4] = 0x510e527f;
    s.h[5] = 0x9b05688c;
    s.h[6] = 0x1f83d9ab;
    s.h[7] = 0x5be0cd19;
    return s;
}

__device__ void sha256_compress(Sha256State& state, uint32_t* block)
{
    uint32_t W[64];
    for (int i = 0; i < 16; i++)
        W[i] = block[i];
    for (int i = 16; i < 64; i++)
        W[i] = gamma1(W[i - 2]) + W[i - 7] + gamma0(W[i - 15]) + W[i - 16];

    uint32_t a = state.h[0], b = state.h[1], c = state.h[2], d = state.h[3];
    uint32_t e = state.h[4], f = state.h[5], g = state.h[6], h = state.h[7];

    for (int i = 0; i < 64; i++)
    {
        uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + W[i];
        uint32_t t2 = sigma0(a) + maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state.h[0] += a;
    state.h[1] += b;
    state.h[2] += c;
    state.h[3] += d;
    state.h[4] += e;
    state.h[5] += f;
    state.h[6] += g;
    state.h[7] += h;
}

__device__ void sha256_hash(const uint8_t* msg, uint32_t msg_len, uint8_t* out_hash)
{
    Sha256State state = sha256_init();

    uint32_t full_blocks = msg_len / 64;
    for (uint32_t b = 0; b < full_blocks; b++)
    {
        uint32_t block[16];
        for (int i = 0; i < 16; i++)
        {
            uint32_t offset = b * 64 + i * 4;
            block[i] = (static_cast<uint32_t>(msg[offset]) << 24) |
                (static_cast<uint32_t>(msg[offset + 1]) << 16) |
                (static_cast<uint32_t>(msg[offset + 2]) << 8) |
                static_cast<uint32_t>(msg[offset + 3]);
        }
        sha256_compress(state, block);
    }

    uint32_t remainder = msg_len % 64;
    uint8_t last_block_bytes[128];
    for (uint32_t i = 0; i < remainder; i++)
        last_block_bytes[i] = msg[full_blocks * 64 + i];
    last_block_bytes[remainder] = 0x80;
    for (uint32_t i = remainder + 1; i < 128; i++)
        last_block_bytes[i] = 0;

    uint32_t total_pad_blocks;
    if (remainder < 56)
    {
        total_pad_blocks = 1;
        uint64_t bit_len = static_cast<uint64_t>(msg_len) * 8;
        last_block_bytes[56] = static_cast<uint8_t>(bit_len >> 56);
        last_block_bytes[57] = static_cast<uint8_t>(bit_len >> 48);
        last_block_bytes[58] = static_cast<uint8_t>(bit_len >> 40);
        last_block_bytes[59] = static_cast<uint8_t>(bit_len >> 32);
        last_block_bytes[60] = static_cast<uint8_t>(bit_len >> 24);
        last_block_bytes[61] = static_cast<uint8_t>(bit_len >> 16);
        last_block_bytes[62] = static_cast<uint8_t>(bit_len >> 8);
        last_block_bytes[63] = static_cast<uint8_t>(bit_len);
    }
    else
    {
        total_pad_blocks = 2;
        uint64_t bit_len = static_cast<uint64_t>(msg_len) * 8;
        last_block_bytes[120] = static_cast<uint8_t>(bit_len >> 56);
        last_block_bytes[121] = static_cast<uint8_t>(bit_len >> 48);
        last_block_bytes[122] = static_cast<uint8_t>(bit_len >> 40);
        last_block_bytes[123] = static_cast<uint8_t>(bit_len >> 32);
        last_block_bytes[124] = static_cast<uint8_t>(bit_len >> 24);
        last_block_bytes[125] = static_cast<uint8_t>(bit_len >> 16);
        last_block_bytes[126] = static_cast<uint8_t>(bit_len >> 8);
        last_block_bytes[127] = static_cast<uint8_t>(bit_len);
    }

    for (uint32_t pb = 0; pb < total_pad_blocks; pb++)
    {
        uint32_t block[16];
        for (int i = 0; i < 16; i++)
        {
            uint32_t offset = pb * 64 + i * 4;
            block[i] = (static_cast<uint32_t>(last_block_bytes[offset]) << 24) |
                (static_cast<uint32_t>(last_block_bytes[offset + 1]) << 16) |
                (static_cast<uint32_t>(last_block_bytes[offset + 2]) << 8) |
                static_cast<uint32_t>(last_block_bytes[offset + 3]);
        }
        sha256_compress(state, block);
    }

    for (int i = 0; i < 8; i++)
    {
        out_hash[i * 4] = static_cast<uint8_t>(state.h[i] >> 24);
        out_hash[i * 4 + 1] = static_cast<uint8_t>(state.h[i] >> 16);
        out_hash[i * 4 + 2] = static_cast<uint8_t>(state.h[i] >> 8);
        out_hash[i * 4 + 3] = static_cast<uint8_t>(state.h[i]);
    }
}

__device__ void double_sha256(const uint8_t* msg, uint32_t msg_len, uint8_t* out_hash)
{
    uint8_t first_hash[32];
    sha256_hash(msg, msg_len, first_hash);
    sha256_hash(first_hash, 32, out_hash);
}

__global__ void mine_kernel(
    const uint8_t* __restrict__ header_prefix,
    const uint32_t* __restrict__ prefix_len,
    const uint8_t* __restrict__ target,
    uint32_t* result_found,
    uint64_t* result_nonce,
    const uint64_t* __restrict__ nonce_offset)
{
    if (atomicAdd(result_found, 0u) != 0)
        return;

    uint64_t nonce = static_cast<uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x + *nonce_offset;

    uint8_t header[320];
    uint32_t plen = *prefix_len;
    for (uint32_t i = 0; i < plen; i++)
        header[i] = header_prefix[i];

    header[plen] = static_cast<uint8_t>(nonce);
    header[plen + 1] = static_cast<uint8_t>(nonce >> 8);
    header[plen + 2] = static_cast<uint8_t>(nonce >> 16);
    header[plen + 3] = static_cast<uint8_t>(nonce >> 24);
    header[plen + 4] = static_cast<uint8_t>(nonce >> 32);
    header[plen + 5] = static_cast<uint8_t>(nonce >> 40);
    header[plen + 6] = static_cast<uint8_t>(nonce >> 48);
    header[plen + 7] = static_cast<uint8_t>(nonce >> 56);

    uint32_t total_len = plen + 8;

    uint8_t hash[32];
    double_sha256(header, total_len, hash);

    bool valid = false;
    for (int i = 0; i < 32; i++)
    {
        if (hash[i] < target[i])
        {
            valid = true;
            break;
        }
        if (hash[i] > target[i])
            break;
    }

    if (valid)
    {
        uint32_t expected = 0;
        if (atomicCAS(result_found, expected, 1u) == 0)
        {
            result_nonce[0] = nonce;
        }
    }
}

CudaMiningBackend::CudaMiningBackend()
{
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    if (err != cudaSuccess || device_count == 0)
    {
        LOG_WARN("CUDA: No GPU device found ({})", cudaGetErrorString(err));
        return;
    }

    err = cudaSetDevice(0);
    if (err != cudaSuccess)
    {
        LOG_WARN("CUDA: Failed to set device ({})", cudaGetErrorString(err));
        return;
    }

    cudaDeviceProp props;
    err = cudaGetDeviceProperties(&props, 0);
    if (err != cudaSuccess)
    {
        LOG_WARN("CUDA: Failed to get device properties ({})", cudaGetErrorString(err));
        return;
    }

    device_id_ = 0;
    available_ = true;
    LOG_INFO("CUDA mining backend initialized: {}", props.name);
}

CudaMiningBackend::~CudaMiningBackend()
{
    if (available_)
    {
        cudaDeviceReset();
    }
}

std::string CudaMiningBackend::name() const
{
    return "cuda";
}

bool CudaMiningBackend::is_available() const
{
    return available_;
}

MineResult CudaMiningBackend::mine(const std::vector<uint8_t>& header_prefix,
    const std::vector<uint8_t>& target_bytes, std::atomic_bool& interrupt)
{
    MineResult result;
    if (!available_)
        return result;

    const uint32_t prefix_len = static_cast<uint32_t>(header_prefix.size());

    uint8_t* d_prefix = nullptr;
    uint32_t* d_prefix_len = nullptr;
    uint8_t* d_target = nullptr;
    uint32_t* d_result_found = nullptr;
    uint64_t* d_result_nonce = nullptr;
    uint64_t* d_nonce_offset = nullptr;

    auto cleanup = [&]()
    {
        cudaFree(d_prefix);
        cudaFree(d_prefix_len);
        cudaFree(d_target);
        cudaFree(d_result_found);
        cudaFree(d_result_nonce);
        cudaFree(d_nonce_offset);
    };

    if (cudaMalloc(&d_prefix, prefix_len) != cudaSuccess ||
        cudaMalloc(&d_prefix_len, sizeof(uint32_t)) != cudaSuccess ||
        cudaMalloc(&d_target, 32) != cudaSuccess ||
        cudaMalloc(&d_result_found, sizeof(uint32_t)) != cudaSuccess ||
        cudaMalloc(&d_result_nonce, sizeof(uint64_t)) != cudaSuccess ||
        cudaMalloc(&d_nonce_offset, sizeof(uint64_t)) != cudaSuccess)
    {
        LOG_WARN("CUDA: Failed to allocate device memory");
        cleanup();
        return result;
    }

    if (cudaMemcpy(d_prefix, header_prefix.data(), prefix_len, cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_prefix_len, &prefix_len, sizeof(uint32_t), cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_target, target_bytes.data(), 32, cudaMemcpyHostToDevice) != cudaSuccess)
    {
        LOG_WARN("CUDA: Failed to copy data to device");
        cleanup();
        return result;
    }

    constexpr uint32_t threads_per_block = 256;
    constexpr uint32_t blocks = BATCH_SIZE / threads_per_block;

    uint64_t nonce_offset = 0;
    uint64_t total_hash_count = 0;

    while (!interrupt)
    {
        uint32_t zero = 0;
        cudaMemcpy(d_result_found, &zero, sizeof(uint32_t), cudaMemcpyHostToDevice);
        cudaMemcpy(d_nonce_offset, &nonce_offset, sizeof(uint64_t), cudaMemcpyHostToDevice);

        KERNEL_LAUNCH(mine_kernel, blocks, threads_per_block,
            d_prefix, d_prefix_len, d_target,
            d_result_found, d_result_nonce, d_nonce_offset);

        cudaError_t err = cudaDeviceSynchronize();
        if (err != cudaSuccess)
        {
            LOG_WARN("CUDA: Kernel execution failed ({})", cudaGetErrorString(err));
            break;
        }

        total_hash_count += BATCH_SIZE;

        uint32_t found_flag = 0;
        cudaMemcpy(&found_flag, d_result_found, sizeof(uint32_t), cudaMemcpyDeviceToHost);
        if (found_flag != 0)
        {
            uint64_t found_nonce = 0;
            cudaMemcpy(&found_nonce, d_result_nonce, sizeof(uint64_t), cudaMemcpyDeviceToHost);
            result.found = true;
            result.nonce = found_nonce;
            result.hash_count = total_hash_count;
            break;
        }

        nonce_offset += BATCH_SIZE;

        if (nonce_offset < BATCH_SIZE)
            break;
    }

    result.hash_count = total_hash_count;

    cleanup();
    return result;
}

#endif // TINY_COIN_CUDA
