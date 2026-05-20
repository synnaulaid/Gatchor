#include "gatchor.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    std::string input;

    std::cout << "Gatchor256 Interactive Hash\n";
    std::cout << "Type 'exit' to quit.\n";

    while (true) {
        std::cout << "\nInput: ";
        std::getline(std::cin, input);

        if (input == "exit") break;

        std::vector<uint8_t> data(input.begin(), input.end());
        std::string hash = gatchor::Gatchor256::hash(data);

        std::cout << "Hash: " << hash << std::endl;
    }

    return 0;
}