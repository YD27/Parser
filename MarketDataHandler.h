#ifndef MARKET_DATA_HANDLER_H
#define MARKET_DATA_HANDLER_H

#include "SocketConnection.h"
#include "OrderBook.h"
#include "CircularBuffer.h"
#include <thread>
#include <atomic>
#include <fstream>

class MarketDataHandler {
private:
    #pragma pack(push, 1)  // Ensure no padding in the structure
    typedef struct {
        short msg_len;   // 2 bytes
        short stream_id; // 2 bytes
        int seq_no;      // 4 bytes
    } STREAM_HEADER;
    #pragma pack(pop)

    struct FEED{
        char msg_type;
        unsigned long timestamp;
        double order_id = -1; //This wouldn't be filled in case of Trade Messages
        double buy_order_id = -1, sell_order_id = -1; //This would be filled in case of Trade Messages
        int token;
        char orderType = '\0'; //This wouldn't be filled in case of Trade Messages
        int price;
        int quantity;
    };

    SocketConnection connection;
    OrderBook orderBook;
    CircularBuffer<FEED> dataBuffer;
    std::thread connectionThread;
    std::thread processingThread;
    std::atomic<bool> running;

    std::unordered_map<int, std::unique_ptr<OrderBook>> orderBookMap; //token to OrderBook mapping
    std::ofstream feedLogFile;

    void processMarketData();
    void listenToMarketData();
    void processFeed(char msg_type, char* recvBuffer, FEED& feed);
    void writeToLogFile(const FEED& feed);

public:
    MarketDataHandler(const std::string& server, int port, size_t bufferSize);
    ~MarketDataHandler();

    void start();
    void stop();
};

#endif // MARKET_DATA_HANDLER_H
