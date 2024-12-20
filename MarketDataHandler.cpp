#include "MarketDataHandler.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include "zstd.h"
#include "lzo/lzo1x.h"

MarketDataHandler::MarketDataHandler(const std::string& server, int port, size_t bufferSize)
    : connection(server, port), dataBuffer(bufferSize), running(false) {
    feedLogFile.open("FeedLogFile.zst", std::ios::binary);
    if (!feedLogFile.is_open()) {
        std::cerr << "Error opening file for writing." << std::endl;
    }
}

MarketDataHandler::~MarketDataHandler() {
    std::cout<<"MDH Destructor called"<<std::endl;
    if (feedLogFile.is_open()) {
        feedLogFile.close();
    }
    //stop();
}

void MarketDataHandler::listenToMarketData() {
    
    constexpr size_t headerSize = sizeof(STREAM_HEADER);
    STREAM_HEADER header;
    char headerBuffer[headerSize];

    while (running) {
        if (!connection.isConnectionActive()) {
            try {
                connection.connectToServer();
            } catch (const std::exception& ex) {
                std::cerr << "Connection error: " << ex.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Retry after delay
                continue;
            }
        }

        sockaddr_in senderAddr;
        socklen_t senderAddrLen = sizeof(senderAddr);
        int headerBytesReceived = recvfrom(connection.getSocketFD(), headerBuffer, headerSize, 0,(struct sockaddr*)&senderAddr, &senderAddrLen);
        if(headerBytesReceived == headerSize){
            // Decompress the header
            char decompressedHeader[headerSize];
            lzo_uint decompressedHeaderSize;

            if (lzo1x_decompress((unsigned char*)headerBuffer, headerSize, (unsigned char*)decompressedHeader, &decompressedHeaderSize, nullptr) != LZO_E_OK) {
                std::cerr << "LZO decompression failed for header" << std::endl;
                continue;
            }

            // Parse the decompressed header
            std::memcpy(&header, decompressedHeader, headerSize);
            // header.msgLength = ntohs(header.msgLength);

            //Receive Rest of Data based on message length received in header
            size_t bufferSize = header.msg_len - headerSize;
            char* recvBuffer = new char[bufferSize];

            int bytesReceived = recvfrom(connection.getSocketFD(), recvBuffer, bufferSize, 0, (struct sockaddr*)&senderAddr, &senderAddrLen);
            if (bytesReceived == bufferSize) {
                // Decompress LZO packet
                char* decompressedBuffer = new char[bufferSize];
                lzo_uint decompressedSize;
                if(lzo1x_decompress((unsigned char*)recvBuffer, bytesReceived, (unsigned char*)decompressedBuffer, &decompressedSize, nullptr) == LZO_E_OK){
                    FEED feed;
                    feed.msg_type = decompressedBuffer[0];
                    processFeed(feed.msg_type, decompressedBuffer, feed);
                    writeToLogFile(feed);
                }
                else{
                    std::cerr << "LZO decompression failed for data" << std::endl;
                    continue;
                }
                delete[] decompressedBuffer;
            }
            else {
                // Error occurred
                std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
                connection.closeConnection();
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Retry after delay
            }
            delete[] recvBuffer;
        }
        else if(headerBytesReceived==0){
            // Server closed the connection
            std::cerr << "Server closed connection. Attempting to reconnect..." << std::endl;
            connection.closeConnection();
        }
        else {
            // Error occurred
            std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
            connection.closeConnection();
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Retry after delay
        }

    }
}

void MarketDataHandler::processFeed(char msg_type,char* recvBuffer, FEED& feed){
    if (msg_type == 'N' || msg_type == 'M' || msg_type == 'X') {
        std::memcpy(&feed.timestamp, recvBuffer + 1, sizeof(unsigned long));
        std::memcpy(&feed.order_id, recvBuffer + 1 + sizeof(unsigned long), sizeof(double));
        std::memcpy(&feed.token, recvBuffer + 1 + sizeof(unsigned long) + sizeof(double), sizeof(int));
        std::memcpy(&feed.orderType, recvBuffer + 1 + sizeof(unsigned long) + sizeof(double) + sizeof(int), sizeof(char));
        std::memcpy(&feed.price, recvBuffer + 1 + sizeof(unsigned long) + sizeof(double) + sizeof(int) + sizeof(char), sizeof(int));
        std::memcpy(&feed.quantity, recvBuffer + 1 + sizeof(unsigned long) + sizeof(double) + sizeof(int) + sizeof(char) + sizeof(int), sizeof(int));
    } else if (msg_type == 'T' || msg_type == 'C') {
        std::memcpy(&feed.timestamp, recvBuffer + 1, sizeof(unsigned long));
        std::memcpy(&feed.buy_order_id, recvBuffer + 1 + sizeof(unsigned long), sizeof(double));
        std::memcpy(&feed.sell_order_id, recvBuffer + 1 + sizeof(unsigned long) + sizeof(double), sizeof(double));
        std::memcpy(&feed.token, recvBuffer + 1 + sizeof(unsigned long) + sizeof(double) + sizeof(double), sizeof(int));
        std::memcpy(&feed.price, recvBuffer + 1 + sizeof(unsigned long) + sizeof(double) + sizeof(double) + sizeof(int), sizeof(int));
        std::memcpy(&feed.quantity, recvBuffer + 1 + sizeof(unsigned long) + sizeof(double) + sizeof(double) + sizeof(int) + sizeof(int), sizeof(int));
    }
    dataBuffer.push(feed);
}

void MarketDataHandler::writeToLogFile(const FEED& feed){
    if(feedLogFile.is_open()){
        std::string logData;

        if (feed.msg_type == 'N' || feed.msg_type== 'M' || feed.msg_type == 'X') {
            logData.append(reinterpret_cast<const char*>(&feed.msg_type), sizeof(feed.msg_type));
            logData.append(reinterpret_cast<const char*>(&feed.timestamp), sizeof(feed.timestamp));
            logData.append(reinterpret_cast<const char*>(&feed.order_id), sizeof(feed.order_id));
            logData.append(reinterpret_cast<const char*>(&feed.token), sizeof(feed.token));
            logData.append(reinterpret_cast<const char*>(&feed.orderType), sizeof(feed.orderType));
            logData.append(reinterpret_cast<const char*>(&feed.price), sizeof(feed.price));
            logData.append(reinterpret_cast<const char*>(&feed.quantity), sizeof(feed.quantity));
        } else if (feed.msg_type == 'T' || feed.msg_type == 'C') {
            logData.append(reinterpret_cast<const char*>(&feed.msg_type), sizeof(feed.msg_type));
            logData.append(reinterpret_cast<const char*>(&feed.timestamp), sizeof(feed.timestamp));
            logData.append(reinterpret_cast<const char*>(&feed.buy_order_id), sizeof(feed.buy_order_id));
            logData.append(reinterpret_cast<const char*>(&feed.sell_order_id), sizeof(feed.sell_order_id));
            logData.append(reinterpret_cast<const char*>(&feed.token), sizeof(feed.token));
            logData.append(reinterpret_cast<const char*>(&feed.price), sizeof(feed.price));
            logData.append(reinterpret_cast<const char*>(&feed.quantity), sizeof(feed.quantity));
        } else {
            std::cerr << "Other Message Type" << std::endl;
            return;
        }

        size_t compressedSize = ZSTD_compressBound(logData.size());
        char* compressedBuffer = new char[compressedSize];

        compressedSize = ZSTD_compress(compressedBuffer, compressedSize, logData.data(), logData.size(), 1);
        if (ZSTD_isError(compressedSize)) {
            std::cerr << "Zstd compression failed" << std::endl;
            delete[] compressedBuffer;
            return;
        }

        feedLogFile.write(compressedBuffer, compressedSize);
        delete[] compressedBuffer;
    }
    else{
        std::cerr << "Error writing to file" << std::endl;
    }
}


void MarketDataHandler::processMarketData() {
    while (running) {
        auto data = dataBuffer.pop();
        if (data) {
            const FEED& feed = *data;
            //Parse relevant data from feed structure and update OrderBook

            // If the token's order book does not exist, create it
            if (orderBookMap.find(feed.token) == orderBookMap.end()) {
                orderBookMap[feed.token] = std::make_unique<OrderBook>(); 
            }
            orderBookMap[feed.token]->processMessage(feed.msg_type, feed.price, feed.quantity, feed.timestamp, feed.token, feed.order_id, feed.buy_order_id,
            feed.sell_order_id, feed.orderType );
        }
    }
}

void MarketDataHandler::start() {
    std::cout<<"MDH Start called"<<std::endl;
    running = true;

    //Connection Thread Actively listens to Market Data and pushes it into Data Buffer
    //Processing Thread fetches Data from Data Buffer and processes it.
    connectionThread = std::thread(&MarketDataHandler::listenToMarketData, this);
    // processingThread = std::thread(&MarketDataHandler::processMarketData, this);
}

void MarketDataHandler::stop() {
    std::cout<<"MDH Stop called"<<std::endl;
    running = false;
    if (connectionThread.joinable()) connectionThread.join();
    //if (processingThread.joinable()) processingThread.join();
    connection.closeConnection();
}
