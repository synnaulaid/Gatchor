#include "gatchor.hpp"
#include <iostream>
#include <vector>
#include <bitset>
#include <random>
#include <unordered_set>

using namespace gatchor;

void avalanche_test(const std::vector<uint8_t>& input) {
    std::string orig = Gatchor256::hash(input);
    std::cout << "Avalanche test for input: '" << std::string(input.begin(), input.end()) << "'\n";

    for (size_t i = 0; i < input.size(); ++i) {
        std::vector<uint8_t> copy = input;
        copy[i] ^= 0x01; // flip 1 bit
        std::string h = Gatchor256::hash(copy);

        int diff_bits = 0;
        for (size_t j = 0; j < h.size(); ++j) {
            uint8_t a = orig[j] ^ h[j];
            diff_bits += std::bitset<8>(a).count();
        }
        std::cout << "Bit flipped at byte " << i << " → " << diff_bits << "/256 bits changed\n";
    }
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

void bit_distribution_test(size_t num_inputs = 1000) {
    std::vector<int> bit_count(256, 0);
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint8_t> dist(0, 255);

    for (size_t i = 0; i < num_inputs; ++i) {
        std::vector<uint8_t> data(32);
        for (auto& b : data) b = dist(rng);
        std::string h = Gatchor256::hash(data);

        for (size_t j = 0; j < h.size(); ++j) {
            std::bitset<8> bits(h[j]);
            for (size_t k = 0; k < 8; ++k) bit_count[j*8 + k] += bits[k];
        }
    }

    std::cout << "\nBit distribution test (" << num_inputs << " random inputs)...\n";
    for (size_t i = 0; i < bit_count.size(); ++i) {
        std::cout << "Bit " << i << ": " << bit_count[i] << "/" << num_inputs
                  << " (" << (bit_count[i] * 100.0 / num_inputs) << "%)\n";
    }
}

int main() {
    std::vector<uint8_t> input = {'h','e','l','l','o',' ','w','o','r','l','d'};

    avalanche_test(input);
    collision_test();
    bit_distribution_test();

    return 0;
}