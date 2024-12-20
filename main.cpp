#include "MarketDataHandler.h"
#include <csignal>
#include <iostream>

std::atomic<bool> running(true);

void signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nStopping MarketDataHandler" << std::endl;
        running = false;
    }
}

int main(int argc, char* argv[]) {
    size_t bufferSize  = 1844670; 
    signal(SIGINT, signalHandler);

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <IP_ADDRESS> <PORT>" << std::endl;
        return 1;
    }

    std::string ipAddress = argv[1];
    unsigned int port = std::stoi(argv[2]);

    MarketDataHandler marketDataHandler(ipAddress.c_str(), port, bufferSize);
    marketDataHandler.start();

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    marketDataHandler.stop();

    return 0;
}
