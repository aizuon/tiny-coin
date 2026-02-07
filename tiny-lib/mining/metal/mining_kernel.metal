#include <metal_stdlib>
using namespace metal;

constant uint K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

inline uint rotr(uint x, uint n) { return (x >> n) | (x << (32 - n)); }
inline uint ch(uint x, uint y, uint z) { return (x & y) ^ (~x & z); }
inline uint maj(uint x, uint y, uint z) { return (x & y) ^ (x & z) ^ (y & z); }
inline uint sigma0(uint x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
inline uint sigma1(uint x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
inline uint gamma0(uint x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
inline uint gamma1(uint x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

struct Sha256State {
    uint h[8];
};

Sha256State sha256_init() {
    Sha256State s;
    s.h[0] = 0x6a09e667; s.h[1] = 0xbb67ae85;
    s.h[2] = 0x3c6ef372; s.h[3] = 0xa54ff53a;
    s.h[4] = 0x510e527f; s.h[5] = 0x9b05688c;
    s.h[6] = 0x1f83d9ab; s.h[7] = 0x5be0cd19;
    return s;
}

void sha256_compress(thread Sha256State& state, thread uint* block) {
    uint W[64];
    for (int i = 0; i < 16; i++)
        W[i] = block[i];
    for (int i = 16; i < 64; i++)
        W[i] = gamma1(W[i-2]) + W[i-7] + gamma0(W[i-15]) + W[i-16];

    uint a = state.h[0], b = state.h[1], c = state.h[2], d = state.h[3];
    uint e = state.h[4], f = state.h[5], g = state.h[6], h = state.h[7];

    for (int i = 0; i < 64; i++) {
        uint t1 = h + sigma1(e) + ch(e, f, g) + K[i] + W[i];
        uint t2 = sigma0(a) + maj(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state.h[0] += a; state.h[1] += b; state.h[2] += c; state.h[3] += d;
    state.h[4] += e; state.h[5] += f; state.h[6] += g; state.h[7] += h;
}

void sha256_hash(thread const uchar* msg, uint msg_len, thread uchar* out_hash) {
    Sha256State state = sha256_init();

    uint full_blocks = msg_len / 64;
    for (uint b = 0; b < full_blocks; b++) {
        uint block[16];
        for (int i = 0; i < 16; i++) {
            uint offset = b * 64 + i * 4;
            block[i] = (uint(msg[offset]) << 24) | (uint(msg[offset+1]) << 16) |
                       (uint(msg[offset+2]) << 8) | uint(msg[offset+3]);
        }
        sha256_compress(state, block);
    }

    uint remainder = msg_len % 64;
    uchar last_block_bytes[128];
    for (uint i = 0; i < remainder; i++)
        last_block_bytes[i] = msg[full_blocks * 64 + i];
    last_block_bytes[remainder] = 0x80;
    for (uint i = remainder + 1; i < 128; i++)
        last_block_bytes[i] = 0;

    uint total_pad_blocks;
    if (remainder < 56) {
        total_pad_blocks = 1;
        ulong bit_len = ulong(msg_len) * 8;
        last_block_bytes[56] = uchar(bit_len >> 56);
        last_block_bytes[57] = uchar(bit_len >> 48);
        last_block_bytes[58] = uchar(bit_len >> 40);
        last_block_bytes[59] = uchar(bit_len >> 32);
        last_block_bytes[60] = uchar(bit_len >> 24);
        last_block_bytes[61] = uchar(bit_len >> 16);
        last_block_bytes[62] = uchar(bit_len >> 8);
        last_block_bytes[63] = uchar(bit_len);
    } else {
        total_pad_blocks = 2;
        ulong bit_len = ulong(msg_len) * 8;
        last_block_bytes[120] = uchar(bit_len >> 56);
        last_block_bytes[121] = uchar(bit_len >> 48);
        last_block_bytes[122] = uchar(bit_len >> 40);
        last_block_bytes[123] = uchar(bit_len >> 32);
        last_block_bytes[124] = uchar(bit_len >> 24);
        last_block_bytes[125] = uchar(bit_len >> 16);
        last_block_bytes[126] = uchar(bit_len >> 8);
        last_block_bytes[127] = uchar(bit_len);
    }

    for (uint pb = 0; pb < total_pad_blocks; pb++) {
        uint block[16];
        for (int i = 0; i < 16; i++) {
            uint offset = pb * 64 + i * 4;
            block[i] = (uint(last_block_bytes[offset]) << 24) |
                       (uint(last_block_bytes[offset+1]) << 16) |
                       (uint(last_block_bytes[offset+2]) << 8) |
                       uint(last_block_bytes[offset+3]);
        }
        sha256_compress(state, block);
    }

    for (int i = 0; i < 8; i++) {
        out_hash[i*4]   = uchar(state.h[i] >> 24);
        out_hash[i*4+1] = uchar(state.h[i] >> 16);
        out_hash[i*4+2] = uchar(state.h[i] >> 8);
        out_hash[i*4+3] = uchar(state.h[i]);
    }
}

void double_sha256(thread const uchar* msg, uint msg_len, thread uchar* out_hash) {
    uchar first_hash[32];
    sha256_hash(msg, msg_len, first_hash);
    sha256_hash(first_hash, 32, out_hash);
}

kernel void mine_kernel(
    device const uchar*    header_prefix   [[buffer(0)]],
    device const uint&     prefix_len      [[buffer(1)]],
    device const uchar*    target          [[buffer(2)]],
    device atomic_uint*    result_found    [[buffer(3)]],
    device ulong*          result_nonce    [[buffer(4)]],
    device const ulong&    nonce_offset    [[buffer(5)]],
    uint                   gid             [[thread_position_in_grid]])
{
    if (atomic_load_explicit(result_found, memory_order_relaxed) != 0)
        return;

    ulong nonce = ulong(gid) + nonce_offset;

    uchar header[320];
    uint plen = prefix_len;
    for (uint i = 0; i < plen; i++)
        header[i] = header_prefix[i];

    header[plen]   = uchar(nonce);
    header[plen+1] = uchar(nonce >> 8);
    header[plen+2] = uchar(nonce >> 16);
    header[plen+3] = uchar(nonce >> 24);
    header[plen+4] = uchar(nonce >> 32);
    header[plen+5] = uchar(nonce >> 40);
    header[plen+6] = uchar(nonce >> 48);
    header[plen+7] = uchar(nonce >> 56);

    uint total_len = plen + 8;

    uchar hash[32];
    double_sha256(header, total_len, hash);

    bool valid = false;
    for (int i = 0; i < 32; i++) {
        if (hash[i] < target[i]) {
            valid = true;
            break;
        }
        if (hash[i] > target[i])
            break;
    }

    if (valid) {
        uint expected = 0;
        if (atomic_compare_exchange_weak_explicit(result_found, &expected, 1u,
                memory_order_relaxed, memory_order_relaxed)) {
            result_nonce[0] = nonce;
        }
    }
}
