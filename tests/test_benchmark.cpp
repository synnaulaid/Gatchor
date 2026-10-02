#include "gatchor_pow.hpp"
#include <iostream>
#include <chrono>

using namespace gatchor;

int main() {
    constexpr size_t iterations = 100000;
    PowHeader header;
    header.version = 1;
    header.timestamp = 1700000000;
    header.difficulty = 12;
    Gatchor256::Digest digest{};
    volatile uint8_t checksum = 0;

    for (uint64_t nonce = 0; nonce < 1000; ++nonce) {
        header.nonce = nonce;
        const auto bytes = serialize_header(header);
        Gatchor256::hash_into(bytes, digest);
    }

    const auto start = std::chrono::steady_clock::now();
    for (size_t nonce = 0; nonce < iterations; ++nonce) {
        header.nonce = nonce;
        const auto bytes = serialize_header(header);
        Gatchor256::hash_into(bytes, digest);
        const bool valid = meets_target(digest, header.difficulty);
        checksum = static_cast<uint8_t>(
            checksum ^ digest[0] ^ static_cast<uint8_t>(valid));
    }
    const auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    const double hashes_per_second = iterations / elapsed;
    std::cout << "Mining header: " << POW_HEADER_SIZE << " bytes, "
              << hashes_per_second << " hashes/s, checksum "
              << static_cast<unsigned int>(checksum) << '\n';
    if (checksum == 0xff) std::cerr << "Impossible checksum guard\n";

    return 0;
}