#include "gatchor.hpp"
#include <iostream>
#include <vector>

int main() {
    std::vector<uint8_t> data = {'t','e','s','t'};
    std::string h = gatchor::Gatchor256::hash(data);
    std::cout << "Test hash: " << h << std::endl;
    return 0;
}