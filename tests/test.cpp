#include "gatchor.hpp"
#include <cassert>
#include <iostream>
#include <vector>

int main() {
    assert(gatchor::Gatchor256::rotl(1, 0) == 1);
    assert(gatchor::Gatchor256::rotl(1, 64) == 1);
    assert(gatchor::Gatchor256::rotl(1, 65) == 2);

    std::vector<uint8_t> data = {'t','e','s','t'};
    std::string h = gatchor::Gatchor256::hash(data);
    assert(h == "2b3845225502545aad35f0755ef88d5fef229d7f6ecb6c27253cbc8e7ea43e66");

    for (size_t size : {0U, 1U, 55U, 56U, 63U, 64U, 65U, 127U, 128U}) {
        data.assign(size, 0xA5);
        const std::string vector_hash = gatchor::Gatchor256::hash(data);
        const std::string span_hash = gatchor::Gatchor256::hash(
            std::span<const uint8_t>(data.data(), data.size()));
        gatchor::Gatchor256::Digest digest{};
        gatchor::Gatchor256::hash_into(data, digest);
        assert(vector_hash == span_hash);
        assert(digest.size() == 32);
    }

    std::cout << "Test hash: " << h << std::endl;
    return 0;
}