#include "pch.h"
#include <iostream>

#include "MatchingEngine.h"

int main()
{
    std::cout << "Hello World!\n"; 

    MatchingEngine me;
    me.InitialiseMarkets({"BTC-USD"});

    me.OnOrderPlace({"BTC-USD", 10, 2, OrderType::Bid});
    me.OnOrderPlace({"BTC-USD", 11, 2, OrderType::Bid});
    me.OnOrderPlace({"BTC-USD", 20, 2, OrderType::Ask});
    me.OnOrderPlace({"BTC-USD", 21, 2, OrderType::Ask});

    me.OnOrderPlace({"BTC-USD", 20, 1, OrderType::Bid});
    me.OnOrderPlace({"BTC-USD", 20, 1, OrderType::Bid});

    me.OnOrderPlace({"BTC-USD", 22, 4, OrderType::Bid});
}
