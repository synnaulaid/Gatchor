#include "gatchor.hpp"

#include <bit>
#include <cstring>

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
        for (unsigned int byte = 0; byte < 8; ++byte) {
            m[i] |= static_cast<uint64_t>(block[i * 8 + byte]) << (byte * 8);
        }
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
            v[i] += rotl(m[i], static_cast<unsigned int>((round + i) & 63));
        }
    }

    for (size_t i = 0; i < 8; ++i) {
        state[i] ^= v[i] ^ m[i];
    }
}

void Gatchor256::hash_into(std::span<const uint8_t> data, Digest& digest)
{
    std::array<uint64_t, 8> state = IV;
    const auto* input = data.data();
    size_t remaining = data.size();

    while (remaining >= BLOCK_SIZE) {
        compress(state, input);
        input += BLOCK_SIZE;
        remaining -= BLOCK_SIZE;
    }

    // Padding is kept on the stack, avoiding a full input-sized allocation.
    std::array<uint8_t, BLOCK_SIZE * 2> padded{};
    if (remaining != 0) {
        std::memcpy(padded.data(), input, remaining);
    }
    padded[remaining] = 0x80;
    const size_t length_offset =
        (remaining < 56) ? 56 : BLOCK_SIZE + 56;
    const uint64_t bit_len = static_cast<uint64_t>(data.size()) * 8;
    for (unsigned int byte = 0; byte < 8; ++byte) {
        padded[length_offset + byte] =
            static_cast<uint8_t>(bit_len >> (byte * 8));
    }

    compress(state, padded.data());
    if (length_offset == BLOCK_SIZE + 56) {
        compress(state, padded.data() + BLOCK_SIZE);
    }

    for (size_t i = 0; i < 4; ++i) {
        const uint64_t out = state[i] ^ state[i + 4];
        for (unsigned int byte = 0; byte < 8; ++byte) {
            digest[i * 8 + byte] =
                static_cast<uint8_t>(out >> ((7 - byte) * 8));
        }
    }
}

std::string Gatchor256::hash(std::span<const uint8_t> data)
{
    Digest digest{};
    hash_into(data, digest);

    static constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.resize(digest.size() * 2);
    for (size_t i = 0; i < digest.size(); ++i) {
        result[i * 2] = hex[digest[i] >> 4];
        result[i * 2 + 1] = hex[digest[i] & 0x0f];
    }
    return result;
}

std::string Gatchor256::hash(const std::vector<uint8_t>& data)
{
    return hash(std::span<const uint8_t>(data));
}

}