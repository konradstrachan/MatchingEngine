
#include "pch.h"

#include <iostream>
#include <cassert>

#include "MatchingEngine.h"
#include "TestClient.h"

#define START_TEST( test_name )                                                 \
    bool result{ true };                                                        \
    std::string name{test_name};

#define EXPECTED( var, val )                                                    \
    if( result && var != val )                                                  \
    {                                                                           \
        std::cerr << "Test " << name.c_str()                                    \
                  << " at line [" << __LINE__ << "] failed"                     \
                  << std::endl;                                                 \
        assert(false);                                                          \
        result = false;                                                         \
    }

int main()
{
    const std::string market{"BTC-USD"};

    const auto PlaceOrdersFn = [](MatchingEngine& me, const std::vector<Order>& orders)
    {
        for(const auto& order : orders)
        {
            // Copy so we can move in to the interface
            Order o{order};
            me.OnOrderPlace(std::move(o));
        }
    };

    {
        START_TEST( "Populate simple OB" )
        MatchingEngine me;
        me.InitialiseMarkets({"BTC-USD"});

        TestClient tc;
        me.RegisterEventObserver(&tc);

        std::vector<Order> orders{
            {market, 10, 2, OrderType::Bid},
            {market, 11, 2, OrderType::Bid},
            {market, 20, 2, OrderType::Ask},
            {market, 21, 2, OrderType::Ask}
        };

        PlaceOrdersFn(me, orders);

        EXPECTED(tc.m_matchingEvents.empty(), true);
        EXPECTED(tc.m_orderBookUpdateEvents.size(), 4);
        EXPECTED(tc.m_orderBookUpdateEvents[0].first, 0);
        EXPECTED(tc.m_orderBookUpdateEvents[0].second, orders[0]);
        EXPECTED(tc.m_orderBookUpdateEvents[1].first, 1);
        EXPECTED(tc.m_orderBookUpdateEvents[1].second, orders[1]);
        EXPECTED(tc.m_orderBookUpdateEvents[2].first, 2);
        EXPECTED(tc.m_orderBookUpdateEvents[2].second, orders[2]);
        EXPECTED(tc.m_orderBookUpdateEvents[3].first, 3);
        EXPECTED(tc.m_orderBookUpdateEvents[3].second, orders[3]);
    }

    {
        START_TEST( "Simple matching tests" )
            MatchingEngine me;
        me.InitialiseMarkets({"BTC-USD"});

        TestClient tc;
        me.RegisterEventObserver(&tc);

        std::vector<Order> orders1{
            {market, 10, 2, OrderType::Bid},
            {market, 11, 2, OrderType::Bid},
            {market, 20, 2, OrderType::Ask},
            {market, 21, 2, OrderType::Ask}
        };

        PlaceOrdersFn(me, orders1);

        EXPECTED(tc.m_matchingEvents.empty(), true);
        EXPECTED(tc.m_orderBookUpdateEvents.size(), 4);
        EXPECTED(tc.m_orderBookUpdateEvents[0].first, 0);
        EXPECTED(tc.m_orderBookUpdateEvents[0].second, orders1[0]);
        EXPECTED(tc.m_orderBookUpdateEvents[1].first, 1);
        EXPECTED(tc.m_orderBookUpdateEvents[1].second, orders1[1]);
        EXPECTED(tc.m_orderBookUpdateEvents[2].first, 2);
        EXPECTED(tc.m_orderBookUpdateEvents[2].second, orders1[2]);
        EXPECTED(tc.m_orderBookUpdateEvents[3].first, 3);
        EXPECTED(tc.m_orderBookUpdateEvents[3].second, orders1[3]);

        std::vector<Order> orders2{
            {market, 20, 1, OrderType::Bid},
            {market, 20, 1, OrderType::Bid}
        };

        PlaceOrdersFn(me, orders2);

        EXPECTED(tc.m_matchingEvents.size(), 2);
        //EXPECTED(tc.m_matchingEvents[0], );

        me.OnOrderPlace({market, 22, 4, OrderType::Bid});
    }

    return 0;
}
