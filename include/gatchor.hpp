#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace gatchor {

class Gatchor256 {
public:
    static constexpr size_t BLOCK_SIZE = 64;
    static constexpr size_t ROUNDS = 12;

    static std::string hash(const std::vector<uint8_t>& data);

    static uint64_t rotl(uint64_t x, int r);

private:
    static void compress(
        std::array<uint64_t, 8>& state,
        const uint8_t block[BLOCK_SIZE]
    );
};

}