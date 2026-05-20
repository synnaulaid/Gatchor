#include "gatchor.hpp"
#include <iostream>
#include <vector>
#include <string>

int main()
{
    std::string input = "hello world";

    std::vector<uint8_t> data(
        input.begin(),
        input.end()
    );

    std::string hash =
        gatchor::Gatchor256::hash(data);

    std::cout << "Hash of '" << input << "': " << hash << std::endl;

    // --- multi-input test ---
    std::vector<uint8_t> d1 = {'h','e','l','l','o'};
    std::vector<uint8_t> d2 = {'h','e','l','l','o',' ','w','o','r','l','d'};
    std::vector<uint8_t> d3 = std::vector<uint8_t>(1000, 'a'); // 1000 byte

    std::cout << "d1: " << gatchor::Gatchor256::hash(d1) << std::endl;
    std::cout << "d2: " << gatchor::Gatchor256::hash(d2) << std::endl;
    std::cout << "d3: " << gatchor::Gatchor256::hash(d3) << std::endl;

    return 0;
}