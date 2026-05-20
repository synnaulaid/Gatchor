#include "gatchor.hpp"
#include <iostream>
#include <vector>
#include <chrono>

using namespace gatchor;

int main() {
    std::vector<std::vector<uint8_t>> inputs = {
        std::vector<uint8_t>(16, 'a'),
        std::vector<uint8_t>(256, 'b'),
        std::vector<uint8_t>(1024, 'c'),
        std::vector<uint8_t>(1024*1024, 'd')
    };

    for (auto& data : inputs) {
        auto start = std::chrono::high_resolution_clock::now();
        std::string h = Gatchor256::hash(data);
        auto end = std::chrono::high_resolution_clock::now();

        double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Input size: " << data.size() << " bytes → hash: "
                  << h.substr(0, 16) << "... Elapsed: " << elapsed_ms << " ms\n";
    }

    return 0;
}