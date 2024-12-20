#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <map>
#include <unordered_map>
#include <vector>
#include <string>

class OrderBook {
private:
    size_t maxLevels; //MaxLevels to be taken as input from user to cache top N levels only

    // Active and passive levels
    std::map<int, int, std::greater<int>> bids; // Price -> Total quantity
    std::map<int, int> asks;

    // Order tracking by ID
    std::unordered_map<double, std::pair<double, int>> bidOrders; // Order ID -> (Price, Quantity)
    std::unordered_map<double, std::pair<double, int>> askOrders;

    //Vectors for Caching top N levels as per user input can be added here

public:
    explicit OrderBook(); 

    void processMessage(char type, int price, int quantity, unsigned long timestamp, int token, double order_id, double buy_order_id, double sell_order_id, char side);

};

#endif // ORDER_BOOK_H
