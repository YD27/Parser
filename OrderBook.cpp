#include "OrderBook.h"
#include <iostream>
#include <algorithm>

OrderBook::OrderBook(){}

void OrderBook::processMessage(char type, int price, int quantity, unsigned long timestamp, int token, double order_id, double buy_order_id, double sell_order_id, char side){
    std::map<double, int, std::greater<double>>* map = nullptr;

    if (side == 'B') {
        std::map<int, int, std::greater<int>>* map = &bids;
    } else {
        std::map<int, int>* map = &asks;
    }

    auto& orders = (side == 'B') ? bidOrders : askOrders;

    if (type == 'N') { // New order
        (*map)[price] += quantity;
        orders[order_id] = {price, quantity};
    } 
    else if (type == 'M') { // Modify order
        auto it = orders.find(order_id);
        if (it != orders.end()) {
            auto [oldPrice, oldQty] = it->second;

            // Update the price level for the old order
            (*map)[oldPrice] -= oldQty;
            if ((*map)[oldPrice] == 0) {
                (*map).erase(oldPrice);
            }

            // Update to the new price and quantity
            (*map)[price] += quantity;
            orders[order_id] = {price, quantity};
        }
    } 
    else if (type == 'X') { // Cancel order
        auto it = orders.find(order_id);
        if (it != orders.end()) {
            auto [oldPrice, oldQty] = it->second;

            // Remove the quantity from the price level
            (*map)[oldPrice] -= oldQty;
            if ((*map)[oldPrice] == 0) {
                (*map).erase(oldPrice);
            }

            // Remove the order from the tracking
            orders.erase(it);
        }
    }
    else if (type == 'T'){
        //For Trade Message update both bid and ask maps accordingly,
        //checking for Partil Fills.
    }

}
