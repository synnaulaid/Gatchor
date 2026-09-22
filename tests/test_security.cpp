#include "gatchor.hpp"
#include <bit>
#include <cassert>
#include <iostream>
#include <vector>
#include <random>
#include <unordered_set>

using namespace gatchor;

void avalanche_test(const std::vector<uint8_t>& input) {
    Gatchor256::Digest orig{};
    Gatchor256::hash_into(input, orig);
    size_t total_changed = 0;
    std::cout << "Avalanche test for input: '" << std::string(input.begin(), input.end()) << "'\n";

    for (size_t i = 0; i < input.size(); ++i) {
        std::vector<uint8_t> copy = input;
        copy[i] ^= 0x01; // flip 1 bit
        Gatchor256::Digest digest{};
        Gatchor256::hash_into(copy, digest);

        int diff_bits = 0;
        for (size_t j = 0; j < orig.size(); ++j) {
            diff_bits += std::popcount(static_cast<unsigned int>(orig[j] ^ digest[j]));
        }
        total_changed += static_cast<size_t>(diff_bits);
        std::cout << "Bit flipped at byte " << i << " → " << diff_bits << "/256 bits changed\n";
    }

    const double average = static_cast<double>(total_changed) / (input.size());
    std::cout << "Average avalanche: " << average << "/256 bits changed\n";
    assert(average >= 120.0 && average <= 136.0);
}

void collision_test(size_t num_inputs = 5000) {
    std::unordered_set<std::string> hashes;
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint8_t> dist(0, 255);

    for (size_t i = 0; i < num_inputs; ++i) {
        std::vector<uint8_t> data(16);
        for (auto& b : data) b = dist(rng);
        hashes.insert(Gatchor256::hash(data));
    }

    std::cout << "\nRandom collision test (" << num_inputs << " inputs)...\n";
    std::cout << "Collisions found: " << (num_inputs - hashes.size()) << "\n";
}

void bit_distribution_test(size_t num_inputs = 4096) {
    std::vector<int> bit_count(256, 0);
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint8_t> dist(0, 255);

    for (size_t i = 0; i < num_inputs; ++i) {
        std::vector<uint8_t> data(32);
        for (auto& b : data) b = dist(rng);
        Gatchor256::Digest digest{};
        Gatchor256::hash_into(data, digest);

        for (size_t j = 0; j < digest.size(); ++j) {
            for (size_t k = 0; k < 8; ++k) {
                bit_count[j * 8 + k] += (digest[j] >> k) & 1;
            }
        }
    }

    std::cout << "\nBit distribution test (" << num_inputs << " random inputs)...\n";
    double total_ratio = 0.0;
    for (size_t i = 0; i < bit_count.size(); ++i) {
        const double ratio = bit_count[i] * 100.0 / num_inputs;
        total_ratio += ratio;
        std::cout << "Bit " << i << ": " << bit_count[i] << "/" << num_inputs
                  << " (" << ratio << "%)\n";
        assert(ratio >= 47.0 && ratio <= 53.0);
    }
    assert(total_ratio / 256.0 >= 49.0 && total_ratio / 256.0 <= 51.0);
}

int main() {
    std::vector<uint8_t> input = {'h','e','l','l','o',' ','w','o','r','l','d'};

    avalanche_test(input);
    collision_test();
    bit_distribution_test();

    return 0;
}