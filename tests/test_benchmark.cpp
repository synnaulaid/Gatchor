#include "gatchor.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>

using namespace gatchor;

int main() {
    constexpr size_t iterations = 100000;
    std::vector<uint8_t> header(80);
    for (size_t i = 0; i < header.size(); ++i) header[i] = static_cast<uint8_t>(i);
    Gatchor256::Digest digest{};
    volatile uint8_t checksum = 0;

    for (size_t i = 0; i < 1000; ++i) {
        header[76] = static_cast<uint8_t>(i);
        Gatchor256::hash_into(header, digest);
    }

    const auto start = std::chrono::steady_clock::now();
    for (size_t nonce = 0; nonce < iterations; ++nonce) {
        header[76] = static_cast<uint8_t>(nonce);
        header[77] = static_cast<uint8_t>(nonce >> 8);
        header[78] = static_cast<uint8_t>(nonce >> 16);
        header[79] = static_cast<uint8_t>(nonce >> 24);
        Gatchor256::hash_into(header, digest);
        checksum = static_cast<uint8_t>(checksum ^ digest[0]);
    }
    const auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    const double hashes_per_second = iterations / elapsed;
    std::cout << "Mining header: " << header.size() << " bytes, "
              << hashes_per_second << " hashes/s, checksum "
              << static_cast<unsigned int>(checksum) << '\n';
    if (checksum == 0xff) std::cerr << "Impossible checksum guard\n";

    return 0;
}