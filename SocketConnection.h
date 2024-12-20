#ifndef SOCKET_CONNECTION_H
#define SOCKET_CONNECTION_H

#include <string>

class SocketConnection {
private:
    std::string server;
    int port;
    int socketFD;
    bool connected;

public:
    SocketConnection(const std::string& server, int port);
    ~SocketConnection();

    void connectToServer();
    void closeConnection();
    bool isConnectionActive() const;
    int getSocketFD() const;
};

#endif // SOCKET_CONNECTION_H
