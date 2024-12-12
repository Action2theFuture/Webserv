#include "Server.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string configFile = "config/default.conf";
    if (argc > 1) {
        configFile = argv[1];
    }

    try {
        Server server(configFile);
        server.start();
    }
    catch (const std::exception &e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
