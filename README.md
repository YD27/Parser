Commands:  
g++ -std=c++17 -o FeederExe main.cpp MarketDataHandler.cpp OrderBook.cpp SocketConnection.cpp -L/usr/local/lib -llzo2 -lzstd
./FeederExe

Set up LZO library with commands:  
./configure  
./make  
./make install  

On Linux Machines  
In MarketDataHandler.cpp add:  
#include <cstring>  
replace std::memcpy with memcpy  
