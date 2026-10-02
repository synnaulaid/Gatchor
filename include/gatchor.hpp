#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace gatchor {

// Experimental non-cryptographic hash. Do not use for authentication,
// signatures, commitments, address derivation, or blockchain consensus.
class Gatchor256 {
public:
    static constexpr size_t BLOCK_SIZE = 64;
    static constexpr size_t ROUNDS = 12;
    using Digest = std::array<uint8_t, 32>;

    static std::string hash(const std::vector<uint8_t>& data);
    static std::string hash(std::span<const uint8_t> data);
    static void hash_into(std::span<const uint8_t> data, Digest& digest);

    static constexpr uint64_t rotl(uint64_t x, unsigned int r) noexcept
    {
        return std::rotl(x, static_cast<int>(r & 63U));
    }

private:
    static void compress(
        std::array<uint64_t, 8>& state,
        const uint8_t block[BLOCK_SIZE]
    );
};

}