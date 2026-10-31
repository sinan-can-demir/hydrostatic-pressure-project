#include <iostream>
#include <string>
#include <vector>

#include "Cli.hpp"

int main(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    return cli::run(args, std::cin, std::cout, std::cerr);
}
