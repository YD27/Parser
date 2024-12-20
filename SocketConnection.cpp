#include "SocketConnection.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

SocketConnection::SocketConnection(const std::string& server, int port)
    : server(server), port(port), socketFD(-1), connected(false) {}

SocketConnection::~SocketConnection() {
    closeConnection();
}

void SocketConnection::connectToServer() {
    if (connected) return;

    socketFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFD < 0) {
        throw std::runtime_error("Socket creation failed");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // serverAddr.sin_addr.s_addr = inet_addr(//multicastIP);


    if (inet_pton(AF_INET, server.c_str(), &serverAddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address or address not supported");
    }

    // Set socket options to allow receiving multicast
    int yes = 1;
    if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        throw std::runtime_error("Failed to set socket option SO_REUSEADDR: " + std::string(strerror(errno)));
    }

    // Bind socket to the multicast address and port
    if (bind(socketFD, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Bind failed: " + std::string(strerror(errno)));
    }

    // Join the multicast group
    struct ip_mreq mreq;
    if (inet_pton(AF_INET, server.c_str(), &mreq.imr_multiaddr) <= 0) {
        throw std::runtime_error("Invalid multicast address: " + server);
    }
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);  // Join the group on all interfaces

    if (setsockopt(socketFD, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        throw std::runtime_error("Failed to join multicast group: " + std::string(strerror(errno)));
    }

    // if (connect(socketFD, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
    //     throw std::runtime_error("Connection to server failed");
    // }

    connected = true;
    std::cout << "Connected to " << server << ":" << port << std::endl;
}

void SocketConnection::closeConnection() {
    if (connected) {
        close(socketFD);
        connected = false;
        std::cout << "Connection closed" << std::endl;
    }
}

bool SocketConnection::isConnectionActive() const {
    return connected;
}

int SocketConnection::getSocketFD() const {
    return socketFD;
}
