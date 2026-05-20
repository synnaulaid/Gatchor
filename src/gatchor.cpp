#include "gatchor.hpp"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace gatchor {

static const std::array<uint64_t, 8> IV = {
    0x6A09E667F3BCC908ULL,
    0xBB67AE8584CAA73BULL,
    0x3C6EF372FE94F82BULL,
    0xA54FF53A5F1D36F1ULL,
    0x510E527FADE682D1ULL,
    0x9B05688C2B3E6C1FULL,
    0x1F83D9ABFB41BD6BULL,
    0x5BE0CD19137E2179ULL
};

uint64_t Gatchor256::rotl(uint64_t x, int r)
{
    return (x << r) | (x >> (64 - r));
}

static inline void mix(
    uint64_t& a,
    uint64_t& b,
    uint64_t& c,
    uint64_t& d)
{
    a += b;
    d ^= a;
    d = gatchor::Gatchor256::rotl(d, 32);

    c += d;
    b ^= c;
    b = gatchor::Gatchor256::rotl(b, 24);

    a += b;
    d ^= a;
    d = gatchor::Gatchor256::rotl(d, 16);

    c += d;
    b ^= c;
    b = gatchor::Gatchor256::rotl(b, 63);
}

void Gatchor256::compress(
    std::array<uint64_t, 8>& state,
    const uint8_t block[BLOCK_SIZE])
{
    std::array<uint64_t, 8> m{};

    for (size_t i = 0; i < 8; ++i) {
        std::memcpy(&m[i], block + (i * 8), 8);
    }

    std::array<uint64_t, 8> v = state;

    for (size_t round = 0; round < ROUNDS; ++round) {

        for (size_t i = 0; i < 8; ++i) {
            v[i] ^= m[(i + round) % 8];
        }

        mix(v[0], v[1], v[2], v[3]);
        mix(v[4], v[5], v[6], v[7]);

        mix(v[0], v[2], v[4], v[6]);
        mix(v[1], v[3], v[5], v[7]);

        for (size_t i = 0; i < 8; ++i) {
            v[i] += rotl(m[i], (round + i) % 64);
        }
    }

    for (size_t i = 0; i < 8; ++i) {
        state[i] ^= v[i] ^ m[i];
    }
}

std::string Gatchor256::hash(const std::vector<uint8_t>& data)
{
    std::array<uint64_t, 8> state = IV;

    std::vector<uint8_t> padded = data;

    uint64_t bit_len = padded.size() * 8;

    // # 1: padding 0x80
    padded.push_back(0x80);

    // # 2: padding 0x00 -> 8 byte
    while ((padded.size() % BLOCK_SIZE) != 56) {
        padded.push_back(0x00);
    }

    // # 3: long input (8 byte, little-endian)
    for (int i = 0; i < 8; ++i) {
        padded.push_back((bit_len >> (i * 8)) & 0xFF);
    }

    // # 4: multi-block compress
    for (size_t i = 0; i < padded.size(); i += BLOCK_SIZE) {
        compress(state, &padded[i]);
    }

    // # 5: fold state for 256-bit output
    std::stringstream ss;
    for (int i = 0; i < 4; ++i) {
        uint64_t out = state[i] ^ state[i + 4];
        ss << std::hex
           << std::setw(16)
           << std::setfill('0')
           << out;
    }

    return ss.str();
}

}